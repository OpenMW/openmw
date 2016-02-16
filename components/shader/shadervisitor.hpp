#ifndef OPENMW_COMPONENTS_SHADERVISITOR_H
#define OPENMW_COMPONENTS_SHADERVISITOR_H

#include <osg/NodeVisitor>

namespace Shader
{

    class ShaderManager;

    /// @brief Adjusts the given subgraph to render using shaders.
    class ShaderVisitor : public osg::NodeVisitor
    {
    public:
        ShaderVisitor(ShaderManager& shaderManager, const std::string& defaultVsTemplate, const std::string& defaultFsTemplate);

        virtual void apply(osg::Node& node);

        virtual void apply(osg::Drawable& drawable);
        virtual void apply(osg::Geometry& geometry);

        void applyStateSet(osg::StateSet* stateset);

        void pushRequirements();
        void popRequirements();

    private:
        ShaderManager& mShaderManager;

        struct ShaderRequirements
        {
            ShaderRequirements();

            // <texture stage, texture name>
            std::map<int, std::string> mTextures;

            bool mColorMaterial;
            // osg::Material::ColorMode
            int mVertexColorMode;

            // -1 == no tangents required
            int mTexStageRequiringTangents;
        };
        std::vector<ShaderRequirements> mRequirements;

        std::string mDefaultVsTemplate;
        std::string mDefaultFsTemplate;

        void createProgram(const ShaderRequirements& reqs, osg::StateSet* stateset);
    };

}

#endif
