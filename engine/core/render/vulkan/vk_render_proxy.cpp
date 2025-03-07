#include "vk_render_proxy.h"
#include "vk_renderer.h"
#include "vk_mapping.h"
#include "vk_shader_program.h"
#include "vk_render_state.h"
#include "vk_framebuffer.h"
#include "engine/core/scene/render_node.h"

namespace Echo
{
    VKRenderProxy::VKRenderProxy()
        : RenderProxy()
    {
    }

    void VKRenderProxy::setMesh(MeshPtr mesh)
    {
        m_mesh = mesh;

        if (m_mesh && m_raytracing)
        {
            VKRenderer::instance()->getRayTracer()->createBottomLevelStructure(m_mesh);
        }
    }

    bool VKRenderProxy::createVkPipeline(VKFramebuffer* vkFrameBuffer)
    {
        if (m_vkPipelineInfo.renderPass != vkFrameBuffer->getVkRenderPass() || isVkStateDirty())
        {
			destroyVkPipeline();

			VKShaderProgram* vkShaderProgram = ECHO_DOWN_CAST<VKShaderProgram*>(m_material->getShader());
			if (m_mesh && vkShaderProgram && vkShaderProgram->isLinked())
			{
				VkVertexInputBindingDescription vertexInputBinding = {};
				vertexInputBinding.binding = 0;
				vertexInputBinding.stride = m_mesh->getVertexStride();
				vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

				vector<VkVertexInputAttributeDescription>::type viAttributeDescriptions;
				buildVkVertexInputAttributeDescriptions(vkShaderProgram, m_mesh->getVertexElements(), viAttributeDescriptions);

				VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
				vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
				vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
				vertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexInputBinding;
				vertexInputStateCreateInfo.vertexAttributeDescriptionCount = viAttributeDescriptions.size();
				vertexInputStateCreateInfo.pVertexAttributeDescriptions = viAttributeDescriptions.data();

				VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo = {};
				pipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
				pipelineInputAssemblyStateCreateInfo.topology = VKMapping::mapPrimitiveTopology(m_mesh->getTopologyType());

                // https://gitmemory.com/issue/KhronosGroup/Vulkan-ValidationLayers/2124/678355023
				vector<VkDynamicState>::type dynamicStateEnables = { };

				VkPipelineDynamicStateCreateInfo dynamicState {};
				dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
                dynamicState.pNext = nullptr;
                dynamicState.flags = 0;
				dynamicState.dynamicStateCount = dynamicStateEnables.size();
				dynamicState.pDynamicStates = dynamicStateEnables.data();

				m_vkPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
                m_vkPipelineInfo.pNext = nullptr;
                m_vkPipelineInfo.flags = 0;
				m_vkPipelineInfo.stageCount = vkShaderProgram->getVkShaderStageCreateInfo().size();
				m_vkPipelineInfo.pStages = vkShaderProgram->getVkShaderStageCreateInfo().data();
                m_vkPipelineInfo.pVertexInputState = &vertexInputStateCreateInfo;
                m_vkPipelineInfo.pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo;
                m_vkPipelineInfo.pTessellationState = nullptr;
                m_vkPipelineInfo.pViewportState = vkFrameBuffer->getVkViewportStateCreateInfo();
                m_vkPipelineInfo.pRasterizationState = getVkRasterizationStateCreateInfo();
                m_vkPipelineInfo.pMultisampleState = getVkMultiSampleStateCreateInfo();
				m_vkPipelineInfo.pDepthStencilState = getVkDepthStencilStateCrateInfo();
                m_vkPipelineInfo.pColorBlendState = getVkColorBlendStateCreateInfo();
                m_vkPipelineInfo.pDynamicState = &dynamicState;
				m_vkPipelineInfo.layout = vkShaderProgram->getVkPipelineLayout();
				m_vkPipelineInfo.renderPass = vkFrameBuffer->getVkRenderPass();
                m_vkPipelineInfo.subpass = 0;
                m_vkPipelineInfo.basePipelineHandle = 0;
                m_vkPipelineInfo.basePipelineIndex = 0;

				VKDebug(vkCreateGraphicsPipelines(VKRenderer::instance()->getVkDevice(), VK_NULL_HANDLE, 1, &m_vkPipelineInfo, nullptr, &m_vkPipeline));
			}
        }

        return m_vkPipeline ? true : false;
    }

    void VKRenderProxy::destroyVkPipeline()
    {
        if (m_vkPipeline)
        {
            vkDestroyPipeline(VKRenderer::instance()->getVkDevice(), m_vkPipeline, nullptr);
            m_vkPipeline = VK_NULL_HANDLE;
        }
    }

    bool VKRenderProxy::getVkVertexAttributeBySemantic(VertexSemantic semantic, spirv_cross::Resource& oResource)
    {
        String attributeName = VKMapping::mapVertexSemanticString(semantic);
        VKShaderProgram* vkShaderProgram = ECHO_DOWN_CAST<VKShaderProgram*>(m_material->getShader());
        if (vkShaderProgram && vkShaderProgram->isLinked())
        {
            spirv_cross::ShaderResources vertexShaderResources = vkShaderProgram->getSpirvShaderResources(ShaderProgram::VS);
            for (auto& resource : vertexShaderResources.stage_inputs)
            {
                if (resource.name == attributeName)
                {
                    oResource = resource;
                    return true;
                }
            }
        }

        return false;
    }

    void VKRenderProxy::buildVkVertexInputAttributeDescriptions(VKShaderProgram* vkShaderProgram, const VertexElementList& vertElements, vector<VkVertexInputAttributeDescription>::type& viAttributeDescriptions)
    {
        if (!vertElements.empty())
        {
            const i32 bufferIdx = 1;
            ui32 elementOffset = 0;
            for (size_t i = 0; i < vertElements.size(); i++)
            {
                spirv_cross::Resource spirvResource = {};
                if (getVkVertexAttributeBySemantic(vertElements[i].m_semantic, spirvResource))
                {
                    const spirv_cross::Compiler* compiler = vkShaderProgram->getSpirvShaderCompiler(ShaderProgram::VS);

                    VkVertexInputAttributeDescription attributeDescription;
                    attributeDescription.binding = compiler->get_decoration(spirvResource.id, spv::DecorationBinding);
                    attributeDescription.location = compiler->get_decoration(spirvResource.id, spv::DecorationLocation);
                    attributeDescription.format = VKMapping::mapVertexFormat(vertElements[i].m_pixFmt);
                    attributeDescription.offset = elementOffset;
                    viAttributeDescriptions.emplace_back(attributeDescription);
                }

                elementOffset += PixelUtil::GetPixelBytes(vertElements[i].m_pixFmt);
            }
        }
    }

    void VKRenderProxy::bindRenderState()
    {

    }

    void VKRenderProxy::bindShaderParams(VkCommandBuffer& vkCommandbuffer)
    {
		VKShaderProgram* vkShaderProgram = ECHO_DOWN_CAST<VKShaderProgram*>(m_material->getShader());
		if (vkShaderProgram)
		{
            i32 textureCount = 0;
            for (ShaderProgram::UniformMap& uniformMap : vkShaderProgram->getUniforms())
            {
                for (auto& it : uniformMap)
                {
                    ShaderProgram::UniformPtr uniform = it.second;
                    Material::UniformValue* uniformValue = m_material->getUniform(uniform->m_name);
                    if (uniform->m_type != SPT_TEXTURE)
                    {
                        const void* value = m_node ? m_node->getGlobalUniformValue(uniform->m_name) : nullptr;
                        if (!value) value = uniformValue->getValue();

                        vkShaderProgram->setUniform(uniform->m_name.c_str(), value, uniform->m_type, uniform->m_count);
                    }
                    else
                    {
                        Texture* texture = uniformValue->getTexture();
                        if (texture)
                        {
                            Renderer::instance()->setTexture(textureCount, texture);
                        }

                        vkShaderProgram->setUniform(uniform->m_name.c_str(), &textureCount, uniform->m_type, uniform->m_count);
                        textureCount++;
                    }
                }
            }

            vkShaderProgram->bindUniforms(vkCommandbuffer, m_vkUniformsInstance);
		}
    }

    void VKRenderProxy::bindGeometry(VkCommandBuffer& vkCommandbuffer)
    {
        VKBuffer* vertexBuffer = ECHO_DOWN_CAST<VKBuffer*>(m_mesh->getVertexBuffer());
        if (vertexBuffer)
        {
            VkDeviceSize offsets[1] = { 0 };
            VkBuffer vkBuffer = vertexBuffer->getVkBuffer();
            vkCmdBindVertexBuffers(vkCommandbuffer, 0, 1, &vkBuffer, offsets);
        }

        VKBuffer* indexBuffer = ECHO_DOWN_CAST<VKBuffer*>(m_mesh->getIndexBuffer());
        if (indexBuffer)
        {
            VkIndexType idxType = VKMapping::mapIndexType(m_mesh->getIndexStride());
            vkCmdBindIndexBuffer(vkCommandbuffer, indexBuffer->getVkBuffer(), 0, idxType);
        }
    }

    bool VKRenderProxy::isVkStateDirty()
    {
        if (m_material)
        {
            ShaderProgram* shader = m_material->getShader();
            if (shader)
            {
                if (shader->getBlendState()->isDirty())
                    return true;

                if (shader->getRasterizerState()->isDirty())
                    return true;

                if (shader->getDepthStencilState()->isDirty())
                    return true;

                if (shader->getMultisampleState()->isDirty())
                    return true;
            }
        }

        return false;
    }

    const VkPipelineColorBlendStateCreateInfo* VKRenderProxy::getVkColorBlendStateCreateInfo()
    {
        VKBlendState* vkState = ECHO_DOWN_CAST<VKBlendState*>( m_material->getShader()->getBlendState());
        return vkState->getVkCreateInfo();
    }

    const VkPipelineRasterizationStateCreateInfo* VKRenderProxy::getVkRasterizationStateCreateInfo()
    {
        VKRasterizerState* vkState = ECHO_DOWN_CAST<VKRasterizerState*>(m_material->getRasterizerState());
        return vkState->getVkCreateInfo();
    }

    const VkPipelineDepthStencilStateCreateInfo* VKRenderProxy::getVkDepthStencilStateCrateInfo()
    {
        VKDepthStencilState* vkState = ECHO_DOWN_CAST<VKDepthStencilState*>(m_material->getShader()->getDepthStencilState());
        return vkState ? vkState->getVkCreateInfo() : nullptr;
    }

    const VkPipelineMultisampleStateCreateInfo* VKRenderProxy::getVkMultiSampleStateCreateInfo()
    {
        VKMultisampleState* vkState = ECHO_DOWN_CAST<VKMultisampleState*>(m_material->getShader()->getMultisampleState());
        return vkState->getVkCreateInfo();
    }
}
