#ifndef OPENMW_COMPONENTS_TERRAIN_DRAWABLE_H
#define OPENMW_COMPONENTS_TERRAIN_DRAWABLE_H

#include <osg/Geometry>

namespace osg
{
    class ClusterCullingCallback;
}

namespace osgUtil
{
    class CullVisitor;
}

namespace SceneUtil
{
    class LightListCallback;
}

namespace Terrain
{

    class CompositeMap;
    class CompositeMapRenderer;

    /**
     * Subclass of Geometry that supports built in multi-pass rendering and built in LightListCallback.
     */
    class TerrainDrawable : public osg::Geometry
    {
    public:
        osg::Object* cloneType() const override { return new TerrainDrawable (); }
        osg::Object* clone(const osg::CopyOp& copyop) const override { return new TerrainDrawable (*this,copyop); }
        bool isSameKindAs(const osg::Object* obj) const override { return dynamic_cast<const TerrainDrawable *>(obj)!=nullptr; }
        const char* className() const override { return "TerrainDrawable"; }
        const char* libraryName() const override { return "Terrain"; }

        TerrainDrawable();
        ~TerrainDrawable(); // has to be defined in the cpp file because we only forward declared some members.
        TerrainDrawable(const TerrainDrawable& copy, const osg::CopyOp& copyop);

        void accept(osg::NodeVisitor &nv) override;
        void cull(osgUtil::CullVisitor* cv);

        typedef std::vector<osg::ref_ptr<osg::StateSet> > PassVector;
        void setPasses (const PassVector& passes);

        void setLightListCallback(SceneUtil::LightListCallback* lightListCallback);

        void createClusterCullingCallback();

        void compileGLObjects(osg::RenderInfo& renderInfo) const override;

        void setupWaterBoundingBox(float waterheight, float margin);
        const osg::BoundingBox& getWaterBoundingBox() const { return mWaterBoundingBox; }

        void setCompositeMap(CompositeMap* map) { mCompositeMap = map; }
        void setCompositeMapRenderer(CompositeMapRenderer* renderer) { mCompositeMapRenderer = renderer; }

    private:
        osg::BoundingBox mWaterBoundingBox;
        PassVector mPasses;

        osg::ref_ptr<osg::ClusterCullingCallback> mClusterCullingCallback;

        osg::ref_ptr<SceneUtil::LightListCallback> mLightListCallback;
        osg::ref_ptr<CompositeMap> mCompositeMap;
        osg::ref_ptr<CompositeMapRenderer> mCompositeMapRenderer;
    };

}


#endif
