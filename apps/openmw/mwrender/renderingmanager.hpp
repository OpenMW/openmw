#ifndef OPENMW_MWRENDER_RENDERINGMANAGER_H
#define OPENMW_MWRENDER_RENDERINGMANAGER_H

#include <osg/ref_ptr>
#include <osg/Light>

#include "objects.hpp"

#include "renderinginterface.hpp"

namespace osg
{
    class Group;
}

namespace Resource
{
    class ResourceSystem;
}

namespace osgViewer
{
    class Viewer;
}

namespace ESM
{
    struct Cell;
}

namespace MWRender
{

    class RenderingManager : public MWRender::RenderingInterface
    {
    public:
        RenderingManager(osgViewer::Viewer& viewer, osg::ref_ptr<osg::Group> rootNode, Resource::ResourceSystem* resourceSystem);

        MWRender::Objects& getObjects();
        MWRender::Actors& getActors();

        Resource::ResourceSystem* getResourceSystem();

        void configureAmbient(const ESM::Cell* cell);

        void removeCell(const MWWorld::CellStore* store);

        osg::Vec3f getEyePos();

    private:
        osgViewer::Viewer& mViewer;
        osg::ref_ptr<osg::Group> mRootNode;
        Resource::ResourceSystem* mResourceSystem;

        osg::ref_ptr<osg::Light> mSunLight;

        std::auto_ptr<Objects> mObjects;
    };

}

#endif
