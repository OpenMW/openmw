#ifndef OPENMW_COMPONENTS_TERRAIN_DRAWABLE_H
#define OPENMW_COMPONENTS_TERRAIN_DRAWABLE_H

#include <osg/Geometry>

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
        virtual osg::Object* cloneType() const { return new TerrainDrawable (); }
        virtual osg::Object* clone(const osg::CopyOp& copyop) const { return new TerrainDrawable (*this,copyop); }
        virtual bool isSameKindAs(const osg::Object* obj) const { return dynamic_cast<const TerrainDrawable *>(obj)!=nullptr; }
        virtual const char* className() const { return "TerrainDrawable"; }
        virtual const char* libraryName() const { return "Terrain"; }

        TerrainDrawable() = default;
        ~TerrainDrawable() = default;
        TerrainDrawable(const TerrainDrawable& copy, const osg::CopyOp& copyop);

        virtual void accept(osg::NodeVisitor &nv);
        void cull(osgUtil::CullVisitor* cv);

        typedef std::vector<osg::ref_ptr<osg::StateSet> > PassVector;
        void setPasses (const PassVector& passes);

        void setLightListCallback(SceneUtil::LightListCallback* lightListCallback);

        virtual void compileGLObjects(osg::RenderInfo& renderInfo) const;

        void setCompositeMap(CompositeMap* map) { mCompositeMap = map; }
        void setCompositeMapRenderer(CompositeMapRenderer* renderer) { mCompositeMapRenderer = renderer; }

    private:
        PassVector mPasses;

        osg::ref_ptr<SceneUtil::LightListCallback> mLightListCallback;
        osg::ref_ptr<CompositeMap> mCompositeMap;
        osg::ref_ptr<CompositeMapRenderer> mCompositeMapRenderer;
    };

}


#endif
