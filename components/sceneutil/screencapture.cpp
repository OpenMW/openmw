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
#include <atomic>

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
                if (mAborted)
                    return;

                try
                {
                    (*mImpl)(*mImage, mContextId);
                }
                catch (const std::exception& e)
                {
                    Log(Debug::Error) << "ScreenCaptureWorkItem exception: " << e.what();
                }
            }

            void abort() override
            {
                mAborted = true;
            }

        private:
            const osg::ref_ptr<osgViewer::ScreenCaptureHandler::CaptureOperation> mImpl;
            const osg::ref_ptr<const osg::Image> mImage;
            const unsigned int mContextId;
            std::atomic_bool mAborted {false};
    };
}

namespace SceneUtil
{
    std::string writeScreenshotToFile(const std::string& screenshotPath, const std::string& screenshotFormat,
                                      const osg::Image& image)
    {
        // Count screenshots.
        int shotCount = 0;

        // Find the first unused filename with a do-while
        std::ostringstream stream;
        std::string lastFileName;
        std::string lastFilePath;
        do
        {
            // Reset the stream
            stream.str("");
            stream.clear();

            stream << "screenshot" << std::setw(3) << std::setfill('0') << shotCount++ << "." << screenshotFormat;

            lastFileName = stream.str();
            lastFilePath = screenshotPath + "/" + lastFileName;

        } while (boost::filesystem::exists(lastFilePath));

        boost::filesystem::ofstream outStream;
        outStream.open(boost::filesystem::path(std::move(lastFilePath)), std::ios::binary);

        osgDB::ReaderWriter* readerwriter = osgDB::Registry::instance()->getReaderWriterForExtension(screenshotFormat);
        if (!readerwriter)
        {
            Log(Debug::Error) << "Error: Can't write screenshot, no '" << screenshotFormat << "' readerwriter found";
            return std::string();
        }

        osgDB::ReaderWriter::WriteResult result = readerwriter->writeImage(image, outStream);
        if (!result.success())
        {
            Log(Debug::Error) << "Error: Can't write screenshot: " << result.message() << " code " << result.status();
            return std::string();
        }

        return lastFileName;
    }

    WriteScreenshotToFileOperation::WriteScreenshotToFileOperation(const std::string& screenshotPath,
                                                                   const std::string& screenshotFormat,
                                                                   std::function<void (std::string)> callback)
        : mScreenshotPath(screenshotPath)
        , mScreenshotFormat(screenshotFormat)
        , mCallback(callback)
    {
    }

    void WriteScreenshotToFileOperation::operator()(const osg::Image& image, const unsigned int /*context_id*/)
    {
        std::string fileName;
        try
        {
            fileName = writeScreenshotToFile(mScreenshotPath, mScreenshotFormat, image);
        }
        catch (const std::exception& e)
        {
            Log(Debug::Error) << "Failed to write screenshot to file with path=\"" << mScreenshotPath
                              << "\", format=\"" << mScreenshotFormat << "\": " << e.what();
        }
        if (fileName.empty())
            mCallback("Failed to save screenshot");
        else
            mCallback(fileName + " has been saved");
    }

    AsyncScreenCaptureOperation::AsyncScreenCaptureOperation(osg::ref_ptr<WorkQueue> queue,
                                                             osg::ref_ptr<CaptureOperation> impl)
        : mQueue(std::move(queue)),
          mImpl(std::move(impl))
    {
        assert(mQueue != nullptr);
        assert(mImpl != nullptr);
    }

    AsyncScreenCaptureOperation::~AsyncScreenCaptureOperation()
    {
        stop();
    }

    void AsyncScreenCaptureOperation::stop()
    {
        for (const osg::ref_ptr<SceneUtil::WorkItem>& item : *mWorkItems.lockConst())
            item->abort();

        for (const osg::ref_ptr<SceneUtil::WorkItem>& item : *mWorkItems.lockConst())
            item->waitTillDone();
    }

    void AsyncScreenCaptureOperation::operator()(const osg::Image& image, const unsigned int context_id)
    {
        osg::ref_ptr<SceneUtil::WorkItem> item(new ScreenCaptureWorkItem(mImpl, image, context_id));
        mQueue->addWorkItem(item);
        const auto isDone = [] (const osg::ref_ptr<SceneUtil::WorkItem>& v) { return v->isDone(); };
        const auto workItems = mWorkItems.lock();
        workItems->erase(std::remove_if(workItems->begin(), workItems->end(), isDone), workItems->end());
        workItems->emplace_back(std::move(item));
    }
}
