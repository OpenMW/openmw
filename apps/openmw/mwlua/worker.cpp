#include "worker.hpp"

#include "luamanagerimp.hpp"

#include "apps/openmw/profile.hpp"

#include <components/debug/debuglog.hpp>
#include <components/settings/values.hpp>

#include <osgViewer/Viewer>

namespace MWLua
{
    Worker::Worker(LuaManager& manager, osgViewer::Viewer& viewer)
        : mManager(manager)
        , mViewer(viewer)
    {
        if (Settings::lua().mLuaNumThreads > 0)
            mThread = std::thread([this] { run(); });
    }

    Worker::~Worker()
    {
        if (mThread && mThread->joinable())
        {
            Log(Debug::Error)
                << "Unexpected destruction of LuaWorker; likely there is an unhandled exception in the main thread.";
            join();
        }
    }

    void Worker::allowUpdate()
    {
        if (!mThread)
            return;
        {
            std::lock_guard<std::mutex> lk(mMutex);
            mUpdateRequest = true;
        }
        mCV.notify_one();
    }

    void Worker::finishUpdate()
    {
        if (mThread)
        {
            std::unique_lock<std::mutex> lk(mMutex);
            mCV.wait(lk, [&] { return !mUpdateRequest; });
        }
        else
            update();
    }

    void Worker::join()
    {
        if (mThread)
        {
            {
                std::lock_guard<std::mutex> lk(mMutex);
                mJoinRequest = true;
            }
            mCV.notify_one();
            mThread->join();
        }
    }

    void Worker::update()
    {
        const osg::Timer_t frameStart = mViewer.getStartTick();
        const unsigned int frameNumber = mViewer.getFrameStamp()->getFrameNumber();
        OMW::ScopedProfile<OMW::UserStatsType::Lua> profile(
            frameStart, frameNumber, *osg::Timer::instance(), *mViewer.getViewerStats());

        mManager.update();
    }

    void Worker::run() noexcept
    {
        while (true)
        {
            std::unique_lock<std::mutex> lk(mMutex);
            mCV.wait(lk, [&] { return mUpdateRequest || mJoinRequest; });
            if (mJoinRequest)
                break;

            try
            {
                update();
            }
            catch (std::exception& e)
            {
                Log(Debug::Error) << "Failed to update LuaManager: " << e.what();
            }

            mUpdateRequest = false;
            lk.unlock();
            mCV.notify_one();
        }
    }
}
