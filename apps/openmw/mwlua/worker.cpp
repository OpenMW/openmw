#include "worker.hpp"

#include "luamanagerimp.hpp"

#include "apps/openmw/profile.hpp"

#include <components/debug/debuglog.hpp>
#include <components/settings/values.hpp>

namespace MWLua
{
    namespace
    {
        // Step size (roughly KBytes of GC work) per background-loop lua_gc call.
        // Small enough that finishGc() never waits long for the step in flight.
        constexpr int sGcStepSize = 10;
    }

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

    void Worker::gc()
    {
        const int steps = Settings::lua().mGcStepsPerFrame;
        if (steps <= 0)
            return;
        if (!mThread)
        {
            mManager.gcStep(steps);
            return;
        }
        {
            std::lock_guard<std::mutex> lk(mMutex);
            mGcRequest = true;
        }
        mCV.notify_one();
    }

    void Worker::finishGc()
    {
        if (!mThread)
            return;
        std::unique_lock<std::mutex> lk(mMutex);
        mGcRequest = false;
        mCV.wait(lk, [&] { return !mGcInProgress; });
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
            mCV.wait(lk, [&] { return mUpdateRequest.has_value() || mGcRequest || mJoinRequest; });
            if (mJoinRequest)
                break;

            if (!mUpdateRequest.has_value())
            {
                // GC step with the lock dropped: finishGc() can post its stop request
                // while the step runs. The mutex guards only the flags.
                mGcInProgress = true;
                lk.unlock();
                bool cycleFinished = false;
                try
                {
                    cycleFinished = mManager.gcStep(sGcStepSize);
                }
                catch (const std::exception& e)
                {
                    Log(Debug::Error) << "Failed to run Lua GC step: " << e.what();
                    cycleFinished = true;
                }
                lk.lock();
                mGcInProgress = false;
                if (cycleFinished)
                    mGcRequest = false;
                if (!mGcRequest)
                {
                    lk.unlock();
                    mCV.notify_one();
                }
                continue;
            }

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
