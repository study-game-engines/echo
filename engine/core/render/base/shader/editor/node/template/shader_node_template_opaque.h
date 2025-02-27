#pragma once

#include "engine/core/render/base/shader/shader_program.h"
#include "engine/core/render/base/shader/editor/node/shader_node.h"
#include "shader_node_template.h"

#ifdef ECHO_EDITOR_MODE

namespace Echo
{
    class ShaderNodeTemplateOpaque : public ShaderNodeTemplate
    {
        ECHO_CLASS(ShaderNodeTemplateOpaque, ShaderNodeTemplate)

    public:
        ShaderNodeTemplateOpaque();
        virtual ~ShaderNodeTemplateOpaque() {}

		// name
		virtual QString name() const override { return QStringLiteral("ShaderTemplateOpaque"); }

        // caption
        virtual QString caption() const override { return QStringLiteral("Shader Template"); }

		// category
		virtual QString category() const override { return "skip me"; }

        // is caption visible
        virtual bool captionVisible() const override { return true; }

		// generate code
		virtual bool generateCode(Echo::ShaderCompiler& compiler) override;

    public:
        // Get compiler
        ShaderCompiler* getCompiler();

    public:
        // Domain
        void setDomain();

    private:
        ShaderCompilerOpaque        m_compiler;
    };
}

#endif