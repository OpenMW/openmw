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

        /// By default, only bump mapped objects will have a shader added to them.
        /// Setting force = true will cause all objects to render using shaders, regardless of having a bump map.
        void setForceShaders(bool force);

        /// Set whether lighting is clamped for visual compatibility with the fixed function pipeline.
        void setClampLighting(bool clamp);

        /// By default, only bump mapped objects use per-pixel lighting.
        /// Setting force = true will cause all shaders to use per-pixel lighting, regardless of having a bump map.
        void setForcePerPixelLighting(bool force);

        virtual void apply(osg::Node& node);

        virtual void apply(osg::Drawable& drawable);
        virtual void apply(osg::Geometry& geometry);

        void applyStateSet(osg::StateSet* stateset);

        void pushRequirements();
        void popRequirements();

    private:
        bool mForceShaders;
        bool mClampLighting;
        bool mForcePerPixelLighting;

        ShaderManager& mShaderManager;

        struct ShaderRequirements
        {
            ShaderRequirements();

            // <texture stage, texture name>
            std::map<int, std::string> mTextures;

            bool mHasNormalMap;

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
