#ifndef OPENMW_COMPONENTS_SCENEUTIL_SCREENCAPTURE_H
#define OPENMW_COMPONENTS_SCENEUTIL_SCREENCAPTURE_H

#include <osgViewer/ViewerEventHandlers>

#include <string>

namespace osg
{
    class Image;
}

namespace SceneUtil
{
    void writeScreenshotToFile(const std::string& screenshotPath, const std::string& screenshotFormat,
                               const osg::Image& image);

    class WriteScreenshotToFileOperation : public osgViewer::ScreenCaptureHandler::CaptureOperation
    {
        public:
            WriteScreenshotToFileOperation(const std::string& screenshotPath, const std::string& screenshotFormat);

            void operator()(const osg::Image& image, const unsigned int /*context_id*/) override;

        private:
            const std::string mScreenshotPath;
            const std::string mScreenshotFormat;
    };
}

#endif
