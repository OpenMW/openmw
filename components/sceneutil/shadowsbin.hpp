#ifndef OPENMW_COMPONENTS_SCENEUTIL_SHADOWBIN_H
#define OPENMW_COMPONENTS_SCENEUTIL_SHADOWBIN_H

#include <array>
#include <osgUtil/RenderBin>
#include <unordered_set>

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
    public:
        template <class T>
        using Array = std::array<T, GL_ALWAYS - GL_NEVER + 1>;

        using CastingPrograms = Array<osg::ref_ptr<osg::Program>>;

        META_Object(SceneUtil, ShadowsBin)
        ShadowsBin(const CastingPrograms& castingPrograms);
        ShadowsBin(const ShadowsBin& rhs, const osg::CopyOp& copyop)
            : osgUtil::RenderBin(rhs, copyop)
            , mNoTestStateSet(rhs.mNoTestStateSet)
            , mShaderAlphaTestStateSet(rhs.mShaderAlphaTestStateSet)
            , mAlphaFuncShaders(rhs.mAlphaFuncShaders)
        {
        }

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
            {
            }

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
                return !needShadows() || needTexture() || mAlphaBlendOverride || mAlphaFuncOverride || mMaterialOverride
                    || mImportantState;
            }
        };

        osgUtil::StateGraph* cullStateGraph(osgUtil::StateGraph* sg, osgUtil::StateGraph* root,
            std::unordered_set<osgUtil::StateGraph*>& uninteresting, bool cullFaceOverridden);

    private:
        ShadowsBin() {}

        osg::ref_ptr<osg::StateSet> mNoTestStateSet;
        osg::ref_ptr<osg::StateSet> mShaderAlphaTestStateSet;

        Array<osg::ref_ptr<osg::StateSet>> mAlphaFuncShaders;
    };
}

#endif
