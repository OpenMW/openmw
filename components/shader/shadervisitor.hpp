#ifndef OPENMW_COMPONENTS_SHADERVISITOR_H
#define OPENMW_COMPONENTS_SHADERVISITOR_H

#include <osg/NodeVisitor>
#include <osg/Program>

namespace Resource
{
    class ImageManager;
}

namespace Shader
{

    class ShaderManager;

    /// @brief Adjusts the given subgraph to render using shaders.
    class ShaderVisitor : public osg::NodeVisitor
    {
    public:
        ShaderVisitor(
            ShaderManager& shaderManager, Resource::ImageManager& imageManager, const std::string& defaultShaderPrefix);

        void setProgramTemplate(const osg::Program* programTemplate) { mProgramTemplate = programTemplate; }

        /// By default, only bump mapped objects will have a shader added to them.
        /// Setting force = true will cause all objects to render using shaders, regardless of having a bump map.
        void setForceShaders(bool force);

        /// Set if we are allowed to modify StateSets encountered in the graph (default true).
        /// @par If set to false, then instead of modifying, the StateSet will be cloned and this new StateSet will be
        /// assigned to the node.
        /// @par This option is useful when the ShaderVisitor is run on a "live" subgraph that may have already been
        /// submitted for rendering.
        void setAllowedToModifyStateSets(bool allowed);

        /// Automatically use normal maps if a file with suitable name exists (see normal map pattern).
        void setAutoUseNormalMaps(bool use);

        void setNormalMapPattern(const std::string& pattern);
        void setNormalHeightMapPattern(const std::string& pattern);

        void setAutoUseSpecularMaps(bool use);

        void setSpecularMapPattern(const std::string& pattern);

        void setApplyLightingToEnvMaps(bool apply);

        void setConvertAlphaTestToAlphaToCoverage(bool convert);
        void setAdjustCoverageForAlphaTest(bool adjustCoverage);

        void setSupportsNormalsRT(bool supports) { mSupportsNormalsRT = supports; }

        void setWeatherParticleOcclusion(bool value) { mWeatherParticleOcclusion = value; }

        void apply(osg::Node& node) override;

        void apply(osg::Drawable& drawable) override;
        void apply(osg::Geometry& geometry) override;

        void applyStateSet(osg::ref_ptr<osg::StateSet> stateset, osg::Node& node);

        void pushRequirements(osg::Node& node);
        void popRequirements();

    private:
        bool mForceShaders;
        bool mAllowedToModifyStateSets;

        bool mAutoUseNormalMaps;
        std::string mNormalMapPattern;
        std::string mNormalHeightMapPattern;

        bool mAutoUseSpecularMaps;
        std::string mSpecularMapPattern;

        bool mApplyLightingToEnvMaps;

        bool mConvertAlphaTestToAlphaToCoverage;
        bool mAdjustCoverageForAlphaTest;

        bool mSupportsNormalsRT;
        bool mWeatherParticleOcclusion = false;

        ShaderManager& mShaderManager;
        Resource::ImageManager& mImageManager;

        struct ShaderRequirements
        {
            ShaderRequirements();
            ~ShaderRequirements() = default;

            // <texture stage, texture name>
            std::map<int, std::string> mTextures;

            bool mShaderRequired;

            int mColorMode;

            bool mMaterialOverridden;
            bool mAlphaTestOverridden;
            bool mAlphaBlendOverridden;

            GLenum mAlphaFunc;
            float mAlphaRef;
            bool mAlphaBlend;

            bool mBlendFuncOverridden;
            bool mAdditiveBlending;

            GLenum mSrcBlendFunc;
            GLenum mDstBlendFunc;

            bool mDiffuseHeight; // true if diffuse map has height info in alpha channel
            bool mNormalHeight; // true if normal map has height info in alpha channel
            bool mReconstructNormalZ; // used for red-green normal maps (e.g. BC5)

            // -1 == no tangents required
            int mTexStageRequiringTangents;

            bool mSoftParticles;

            // the Node that requested these requirements
            osg::Node* mNode;
        };
        std::vector<ShaderRequirements> mRequirements;

        std::string mDefaultShaderPrefix;

        void createProgram(const ShaderRequirements& reqs);
        void ensureFFP(osg::Node& node);
        bool adjustGeometry(osg::Geometry& sourceGeometry, const ShaderRequirements& reqs);

        osg::ref_ptr<const osg::Program> mProgramTemplate;
    };

    class ReinstateRemovedStateVisitor : public osg::NodeVisitor
    {
    public:
        ReinstateRemovedStateVisitor(bool allowedToModifyStateSets);

        void apply(osg::Node& node) override;

    private:
        bool mAllowedToModifyStateSets;
    };

}

#endif
