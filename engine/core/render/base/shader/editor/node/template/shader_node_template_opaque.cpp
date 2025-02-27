#include "shader_node_template_opaque.h"

#ifdef ECHO_EDITOR_MODE

namespace Echo
{
	ShaderNodeTemplateOpaque::ShaderNodeTemplateOpaque()
    {
		setDomain();
    }

	void ShaderNodeTemplateOpaque::bindMethods()
	{
	}

	void ShaderNodeTemplateOpaque::setDomain()
	{
		m_inputDataTypes =
		{
			{"vec3", "BaseColor"},
			{"vec3", "Specular"},
			{"vec3", "Normal"},
			{"float", "Metallic"},
			{"float", "Roughness"},
			{"float", "Occlusion"},
			{"vec3", "Emissive"}
		};

		m_inputs.resize(m_inputDataTypes.size());
	}

	ShaderCompiler* ShaderNodeTemplateOpaque::getCompiler()
	{
		return &m_compiler;
	}

    bool ShaderNodeTemplateOpaque::generateCode(Echo::ShaderCompiler& compiler)
    {
        for (size_t i = 0; i < m_inputs.size(); i++)
        {
            if (m_inputs[i])
            {
				if (m_inputDataTypes[i].name == "BaseColor")
				{
					compiler.addMacro("ENABLE_BASECOLOR");
					compiler.addCode(Echo::StringUtil::Format("\tvec3 __BaseColor = %s;\n", dynamic_cast<ShaderData*>(m_inputs[i].get())->getVariableName().c_str()));
				}

				if (m_inputDataTypes[i].name == "Specular")
				{
					compiler.addMacro("ENABLE_SPECULAR");
					compiler.addCode(Echo::StringUtil::Format("\tvec3 __Specular = %s;\n", dynamic_cast<ShaderData*>(m_inputs[i].get())->getVariableName().c_str()));
				}

				if (m_inputDataTypes[i].name == "Opacity")
				{
					compiler.addMacro("ENABLE_OPACITY");
					compiler.addCode(Echo::StringUtil::Format("\tfloat __Opacity = %s;\n", dynamic_cast<ShaderData*>(m_inputs[i].get())->getVariableName().c_str()));
				}

				if (m_inputDataTypes[i].name == "Normal")
				{
                    compiler.addUniform("vec3", "u_CameraPosition");

                    compiler.addMacro("ENABLE_VERTEX_POSITION");
					compiler.addMacro("ENABLE_NORMAL");

                    compiler.addCode(Echo::StringUtil::Format("\tvec3 __Normal = %s;\n", dynamic_cast<ShaderData*>(m_inputs[i].get())->getVariableName().c_str()));
				}

				if (m_inputDataTypes[i].name == "Metallic")
				{
					compiler.addMacro("ENABLE_METALIC");
					compiler.addCode(Echo::StringUtil::Format("\tfloat __Metalic = %s;\n", dynamic_cast<ShaderData*>(m_inputs[i].get())->getVariableName().c_str()));
				}

				if (m_inputDataTypes[i].name == "Roughness")
				{
					compiler.addMacro("ENABLE_ROUGHNESS");
					compiler.addCode(Echo::StringUtil::Format("\tfloat __PerceptualRoughness = %s;\n", dynamic_cast<ShaderData*>(m_inputs[i].get())->getVariableName().c_str()));
				}

				if (m_inputDataTypes[i].name == "Occlusion")
				{
					compiler.addMacro("ENABLE_OCCLUSION");
					compiler.addCode(Echo::StringUtil::Format("\tfloat __AmbientOcclusion = %s;\n", dynamic_cast<ShaderData*>(m_inputs[i].get())->getVariableName().c_str()));
				}

				if (m_inputDataTypes[i].name == "Emissive")
				{
					compiler.addMacro("ENABLE_EMISSIVE");
					compiler.addCode(Echo::StringUtil::Format("\tvec3 __Emissive = %s;\n", dynamic_cast<ShaderData*>(m_inputs[i].get())->getVariableName().c_str()));
				}
            }
        }

        return true;
    }
}

#endif