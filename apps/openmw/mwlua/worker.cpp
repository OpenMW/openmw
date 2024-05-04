#include "worker.hpp"

#include "luamanagerimp.hpp"

#include "apps/openmw/profile.hpp"

#include <components/debug/debuglog.hpp>
#include <components/settings/values.hpp>

#include <cassert>

namespace MWLua
{
    Worker::Worker(LuaManager& manager)
        : mManager(manager)
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

    void Worker::allowUpdate(osg::Timer_t frameStart, unsigned frameNumber, osg::Stats& stats)
    {
        if (!mThread)
            return;
        {
            std::lock_guard<std::mutex> lk(mMutex);
            mUpdateRequest = UpdateRequest{ .mFrameStart = frameStart, .mFrameNumber = frameNumber, .mStats = &stats };
        }
        mCV.notify_one();
    }

    void Worker::finishUpdate(osg::Timer_t frameStart, unsigned frameNumber, osg::Stats& stats)
    {
        if (mThread)
        {
            std::unique_lock<std::mutex> lk(mMutex);
            mCV.wait(lk, [&] { return !mUpdateRequest.has_value(); });
        }
        else
            update(frameStart, frameNumber, stats);
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

    void Worker::update(osg::Timer_t frameStart, unsigned frameNumber, osg::Stats& stats)
    {
        const osg::Timer* const timer = osg::Timer::instance();
        OMW::ScopedProfile<OMW::UserStatsType::Lua> profile(frameStart, frameNumber, *timer, stats);

        mManager.update();
    }

    void Worker::run() noexcept
    {
        while (true)
        {
            std::unique_lock<std::mutex> lk(mMutex);
            mCV.wait(lk, [&] { return mUpdateRequest.has_value() || mJoinRequest; });
            if (mJoinRequest)
                break;

            assert(mUpdateRequest.has_value());

            try
            {
                update(mUpdateRequest->mFrameStart, mUpdateRequest->mFrameNumber, *mUpdateRequest->mStats);
            }
            catch (const std::exception& e)
            {
                Log(Debug::Error) << "Failed to update LuaManager: " << e.what();
            }

            mUpdateRequest.reset();
            lk.unlock();
            mCV.notify_one();
        }
    }
}
