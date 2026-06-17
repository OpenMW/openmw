#ifndef MWRENDER_SCREENSHOTMANAGER_H
#define MWRENDER_SCREENSHOTMANAGER_H

#include <osg/ref_ptr>

#include <osgViewer/Viewer>

namespace MWRender
{
    class NotifyDrawCompletedCallback;

    class ScreenshotManager
    {
    public:
        ScreenshotManager(osgViewer::Viewer* viewer);
        ~ScreenshotManager();

        void screenshot(osg::Image* image, int w, int h);

    private:
        osg::ref_ptr<osgViewer::Viewer> mViewer;
        osg::ref_ptr<NotifyDrawCompletedCallback> mDrawCompleteCallback;
    };
}

#endif
