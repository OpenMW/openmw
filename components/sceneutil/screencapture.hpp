#ifndef OPENMW_COMPONENTS_SCENEUTIL_SCREENCAPTURE_H
#define OPENMW_COMPONENTS_SCENEUTIL_SCREENCAPTURE_H

#include <osg/ref_ptr>
#include <osgViewer/ViewerEventHandlers>

#include <string>

namespace osg
{
    class Image;
}

namespace SceneUtil
{
    class WorkQueue;

    void writeScreenshotToFile(const std::string& screenshotPath, const std::string& screenshotFormat,
                               const osg::Image& image);

    class WriteScreenshotToFileOperation : public osgViewer::ScreenCaptureHandler::CaptureOperation
    {
        public:
            WriteScreenshotToFileOperation(const std::string& screenshotPath, const std::string& screenshotFormat);

            void operator()(const osg::Image& image, const unsigned int context_id) override;

        private:
            const std::string mScreenshotPath;
            const std::string mScreenshotFormat;
    };

    class AsyncScreenCaptureOperation : public osgViewer::ScreenCaptureHandler::CaptureOperation
    {
        public:
            AsyncScreenCaptureOperation(osg::ref_ptr<SceneUtil::WorkQueue> queue,
                                        osg::ref_ptr<osgViewer::ScreenCaptureHandler::CaptureOperation> impl);

            void operator()(const osg::Image& image, const unsigned int context_id) override;

        private:
            const osg::ref_ptr<SceneUtil::WorkQueue> mQueue;
            const osg::ref_ptr<osgViewer::ScreenCaptureHandler::CaptureOperation> mImpl;
    };
}

#endif
