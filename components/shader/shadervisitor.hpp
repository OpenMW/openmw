#ifndef OPENMW_COMPONENTS_SHADERVISITOR_H
#define OPENMW_COMPONENTS_SHADERVISITOR_H

#include <osg/NodeVisitor>

namespace Resource
{
    class ImageManager;
}
namespace osg
{
    class Program;
}

namespace Shader
{

    class ShaderManager;

    /// @brief Adjusts the given subgraph to render using shaders.
    class ShaderVisitor : public osg::NodeVisitor
    {
    public:
        ShaderVisitor(ShaderManager& shaderManager, Resource::ImageManager& imageManager);
        ShaderVisitor(const ShaderVisitor& copy, const osg::CopyOp& copyop=osg::CopyOp());
        ~ShaderVisitor();

        void setDefaultShaderPrefix(const std::string& defaultShaderPrefix) { mDefaultShaderPrefix = defaultShaderPrefix; }

        void setProgramTemplate(const osg::Program* programTemplate) { mProgramTemplate = programTemplate; }
        const osg::Program* getProgramTemplate() const { return mProgramTemplate; }

        /// By default, only bump mapped objects will have a shader added to them.
        /// Setting force = true will cause all objects to render using shaders, regardless of having a bump map.
        void setForceShaders(bool force) { mForceShaders = force; }
        bool getForceShaders() const { return mForceShaders; }

        /// Set if we are allowed to modify StateSets encountered in the graph (default false).
        /// @par If set to false, then instead of modifying, the StateSet will be cloned and this new StateSet will be assigned to the node.
        /// @par Setting this option to true is useful when the ShaderVisitor is run on StateSets that have not been submitted for rendering yet.
        void setAllowedToModifyStateSets(bool allowed) { mAllowedToModifyStateSets = allowed; }

        /// Automatically use texture maps if a file with suitable name exists.
        /// @note An empty pattern string indicates we will not use texture maps automatically.
        enum AutoUsedMap
        {
            NormalMap,
            NormalHeightMap,
            SpecularMap,
            EnumSize
        };
        void setAutoMapPattern(AutoUsedMap map, const std::string& pattern) { mAutoMapPatterns[map] = pattern; }
        const std::string& getAutoMapPattern(AutoUsedMap map) const { return mAutoMapPatterns[map]; }

        void setApplyLightingToEnvMaps(bool apply) { mApplyLightingToEnvMaps = apply; }

        void setConvertAlphaTestToAlphaToCoverage(bool convert) { mConvertAlphaTestToAlphaToCoverage = convert; }

        void apply(osg::Node& node) override;

        void apply(osg::Drawable& drawable) override;
        void apply(osg::Geometry& geometry) override;

        void applyStateSet(osg::ref_ptr<osg::StateSet> stateset, osg::Node& node);

    private:
        void pushRequirements(osg::Node& node);
        void popRequirements();

        bool mForceShaders;
        bool mAllowedToModifyStateSets;

        std::vector<std::string> mAutoMapPatterns;

        bool mApplyLightingToEnvMaps;

        bool mConvertAlphaTestToAlphaToCoverage;

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

            bool mNormalHeight; // true if normal map has height info in alpha channel

            // -1 == no tangents required
            int mTexStageRequiringTangents;

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
