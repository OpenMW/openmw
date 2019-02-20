#ifndef OPENMW_COMPONENTS_SCENEUTIL_SHADOWBIN_H
#define OPENMW_COMPONENTS_SCENEUTIL_SHADOWBIN_H
#include <unordered_set>
#include <osgUtil/RenderBin>

namespace osg
{
    class Material;
}

namespace SceneUtil
{

    /// renderbin which culls redundent state for shadows rendering
    class ShadowsBin : public osgUtil::RenderBin
    {
    private:
        osg::ref_ptr<osg::StateSet> mStateSet;
    public:
         META_Object(SceneUtil, ShadowsBin)
         ShadowsBin();
         ShadowsBin(const ShadowsBin& rhs, const osg::CopyOp& copyop) : osgUtil::RenderBin(rhs, copyop), mStateSet(rhs.mStateSet) {}

         virtual void sortImplementation();

        struct State
        {
            State():mAlphaBlend(false),mAlphaBlendOverride(false),mAlphaTest(false),mAlphaTestOverride(false),mMaterial(nullptr),mMaterialOverride(false),mImportantState(false){}
            bool mAlphaBlend;
            bool mAlphaBlendOverride;
            bool mAlphaTest;
            bool mAlphaTestOverride;
            osg::Material* mMaterial;
            bool mMaterialOverride;
            bool mImportantState;
            bool needTexture() const { return mAlphaBlend || mAlphaTest; }
            bool needShadows() const;
            bool interesting() const { return !needShadows() || needTexture() || mAlphaBlendOverride || mAlphaTestOverride || mMaterialOverride || mImportantState; }
        };

        bool cullStateGraph(osgUtil::StateGraph* sg, osgUtil::StateGraph* root, std::unordered_set<osgUtil::StateGraph*>& uninteresting);

         static void addPrototype(const std::string& name)
         {
             osg::ref_ptr<osgUtil::RenderBin> bin (new ShadowsBin);
             osgUtil::RenderBin::addRenderBinPrototype(name, bin);
         }
    };

    class ShadowsBinAdder
    {
        public:
        ShadowsBinAdder(const std::string& name){ ShadowsBin::addPrototype(name); }
    };

}

#endif
