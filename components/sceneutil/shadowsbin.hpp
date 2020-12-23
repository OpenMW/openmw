#ifndef OPENMW_COMPONENTS_SCENEUTIL_SHADOWBIN_H
#define OPENMW_COMPONENTS_SCENEUTIL_SHADOWBIN_H
#include <array>
#include <unordered_set>
#include <osgUtil/RenderBin>

namespace osg
{
    class Material;
    class AlphaFunc;
}

namespace SceneUtil
{

    /// renderbin which culls redundant state for shadow map rendering
    class ShadowsBin : public osgUtil::RenderBin
    {
    private:
        static std::array<osg::ref_ptr<osg::Program>, GL_ALWAYS - GL_NEVER + 1> sCastingPrograms;

        osg::ref_ptr<osg::StateSet> mNoTestStateSet;
        osg::ref_ptr<osg::StateSet> mShaderAlphaTestStateSet;

        std::array<osg::ref_ptr<osg::StateSet>, GL_ALWAYS - GL_NEVER + 1> mAlphaFuncShaders;
    public:
        META_Object(SceneUtil, ShadowsBin)
        ShadowsBin();
        ShadowsBin(const ShadowsBin& rhs, const osg::CopyOp& copyop)
            : osgUtil::RenderBin(rhs, copyop)
            , mNoTestStateSet(rhs.mNoTestStateSet)
            , mShaderAlphaTestStateSet(rhs.mShaderAlphaTestStateSet)
            , mAlphaFuncShaders(rhs.mAlphaFuncShaders)
            {}

        void sortImplementation() override;

        struct State
        {
            State()
                : mAlphaBlend(false)
                , mAlphaBlendOverride(false)
                , mAlphaFunc(nullptr)
                , mAlphaFuncOverride(false)
                , mMaterial(nullptr)
                , mMaterialOverride(false)
                , mImportantState(false)
                {}

            bool mAlphaBlend;
            bool mAlphaBlendOverride;
            osg::AlphaFunc* mAlphaFunc;
            bool mAlphaFuncOverride;
            osg::Material* mMaterial;
            bool mMaterialOverride;
            bool mImportantState;
            bool needTexture() const;
            bool needShadows() const;
            // A state is interesting if there's anything about it that might affect whether we can optimise child state
            bool interesting() const
            {
                return !needShadows() || needTexture() || mAlphaBlendOverride || mAlphaFuncOverride || mMaterialOverride || mImportantState;
            }
        };

        osgUtil::StateGraph* cullStateGraph(osgUtil::StateGraph* sg, osgUtil::StateGraph* root, std::unordered_set<osgUtil::StateGraph*>& uninteresting, bool cullFaceOverridden);

        static void addPrototype(const std::string& name, const std::array<osg::ref_ptr<osg::Program>, GL_ALWAYS - GL_NEVER + 1>& castingPrograms);
    };

    class ShadowsBinAdder
    {
        public:
        ShadowsBinAdder(const std::string& name, const std::array<osg::ref_ptr<osg::Program>, GL_ALWAYS - GL_NEVER + 1>& castingPrograms){ ShadowsBin::addPrototype(name, castingPrograms); }
    };

}

#endif
