#ifndef MWRENDER_SCREENSHOTMANAGER_H
#define MWRENDER_SCREENSHOTMANAGER_H

#include <memory>

#include <osg/Group>
#include <osg/ref_ptr>

#include <osgViewer/Viewer>

namespace Resource
{
    class ResourceSystem;
}

namespace MWRender
{
    class Water;

    class ScreenshotManager
    {
    public:
        ScreenshotManager(osgViewer::Viewer* viewer, osg::ref_ptr<osg::Group> rootNode, osg::ref_ptr<osg::Group> sceneRoot, Resource::ResourceSystem* resourceSystem, Water* water);

        void screenshot(osg::Image* image, int w, int h);
        bool screenshot360(osg::Image* image);

    private:
        osg::ref_ptr<osgViewer::Viewer> mViewer;
        osg::ref_ptr<osg::Group> mRootNode;
        osg::ref_ptr<osg::Group> mSceneRoot;
        Resource::ResourceSystem* mResourceSystem;
        Water* mWater;

        void renderCameraToImage(osg::Camera *camera, osg::Image *image, int w, int h);
        void makeCubemapScreenshot(osg::Image* image, int w, int h, osg::Matrixd cameraTransform=osg::Matrixd());
    };
}

#endif
