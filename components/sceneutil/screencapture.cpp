#include "screencapture.hpp"

#include <components/debug/debuglog.hpp>
#include <components/sceneutil/workqueue.hpp>

#include <osg/ref_ptr>
#include <osg/Image>
#include <osgDB/ReaderWriter>
#include <osgDB/Registry>

#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>

#include <cassert>
#include <iomanip>
#include <sstream>
#include <string>

namespace
{
    class ScreenCaptureWorkItem : public SceneUtil::WorkItem
    {
        public:
            ScreenCaptureWorkItem(const osg::ref_ptr<osgViewer::ScreenCaptureHandler::CaptureOperation>& impl,
                                  const osg::Image& image, unsigned int contextId)
                : mImpl(impl),
                  mImage(new osg::Image(image)),
                  mContextId(contextId)
            {
                assert(mImpl != nullptr);
            }

            void doWork() override
            {
                try
                {
                    (*mImpl)(*mImage, mContextId);
                }
                catch (const std::exception& e)
                {
                    Log(Debug::Error) << "ScreenCaptureWorkItem exception: " << e.what();
                }
            }

        private:
            const osg::ref_ptr<osgViewer::ScreenCaptureHandler::CaptureOperation> mImpl;
            const osg::ref_ptr<const osg::Image> mImage;
            const unsigned int mContextId;
    };
}

namespace SceneUtil
{
    void writeScreenshotToFile(const std::string& screenshotPath, const std::string& screenshotFormat,
                               const osg::Image& image)
    {
        // Count screenshots.
        int shotCount = 0;

        // Find the first unused filename with a do-while
        std::ostringstream stream;
        do
        {
            // Reset the stream
            stream.str("");
            stream.clear();

            stream << screenshotPath << "/screenshot" << std::setw(3) << std::setfill('0') << shotCount++ << "." << screenshotFormat;

        } while (boost::filesystem::exists(stream.str()));

        boost::filesystem::ofstream outStream;
        outStream.open(boost::filesystem::path(stream.str()), std::ios::binary);

        osgDB::ReaderWriter* readerwriter = osgDB::Registry::instance()->getReaderWriterForExtension(screenshotFormat);
        if (!readerwriter)
        {
            Log(Debug::Error) << "Error: Can't write screenshot, no '" << screenshotFormat << "' readerwriter found";
            return;
        }

        osgDB::ReaderWriter::WriteResult result = readerwriter->writeImage(image, outStream);
        if (!result.success())
        {
            Log(Debug::Error) << "Error: Can't write screenshot: " << result.message() << " code " << result.status();
        }
    }

    WriteScreenshotToFileOperation::WriteScreenshotToFileOperation(const std::string& screenshotPath,
                                                                   const std::string& screenshotFormat)
        : mScreenshotPath(screenshotPath)
        , mScreenshotFormat(screenshotFormat)
    {
    }

    void WriteScreenshotToFileOperation::operator()(const osg::Image& image, const unsigned int /*context_id*/)
    {
        try
        {
            writeScreenshotToFile(mScreenshotPath, mScreenshotFormat, image);
        }
        catch (const std::exception& e)
        {
            Log(Debug::Error) << "Failed to write screenshot to file with path=\"" << mScreenshotPath
                              << "\", format=\"" << mScreenshotFormat << "\": " << e.what();
        }
    }

    AsyncScreenCaptureOperation::AsyncScreenCaptureOperation(osg::ref_ptr<WorkQueue> queue,
                                                             osg::ref_ptr<CaptureOperation> impl)
        : mQueue(std::move(queue)),
          mImpl(std::move(impl))
    {
        assert(mQueue != nullptr);
        assert(mImpl != nullptr);
    }

    void AsyncScreenCaptureOperation::operator()(const osg::Image& image, const unsigned int context_id)
    {
        mQueue->addWorkItem(new ScreenCaptureWorkItem(mImpl, image, context_id));
    }
}
