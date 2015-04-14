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

    class StateUpdater;

    class SkyManager;

    class RenderingManager : public MWRender::RenderingInterface
    {
    public:
        RenderingManager(osgViewer::Viewer& viewer, osg::ref_ptr<osg::Group> rootNode, Resource::ResourceSystem* resourceSystem);
        ~RenderingManager();

        MWRender::Objects& getObjects();
        MWRender::Actors& getActors();

        Resource::ResourceSystem* getResourceSystem();

        void setAmbientColour(const osg::Vec4f& colour);

        void setSunDirection(const osg::Vec3f& direction);
        void setSunColour(const osg::Vec4f& colour);

        void configureAmbient(const ESM::Cell* cell);

        void configureFog(float fogDepth, const osg::Vec4f& colour);

        void removeCell(const MWWorld::CellStore* store);

        void setSkyEnabled(bool enabled);

        SkyManager* getSkyManager();

        osg::Vec3f getEyePos();

    private:
        osgViewer::Viewer& mViewer;
        osg::ref_ptr<osg::Group> mRootNode;
        Resource::ResourceSystem* mResourceSystem;

        osg::ref_ptr<osg::Light> mSunLight;

        std::auto_ptr<Objects> mObjects;
        std::auto_ptr<SkyManager> mSky;

        osg::ref_ptr<StateUpdater> mStateUpdater;

        void operator = (const RenderingManager&);
        RenderingManager(const RenderingManager&);
    };

}

#endif
