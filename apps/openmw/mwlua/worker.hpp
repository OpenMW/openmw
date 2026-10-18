#ifndef OPENMW_MWLUA_WORKER_H
#define OPENMW_MWLUA_WORKER_H

#include <components/misc/framestats.hpp>

#include <condition_variable>
#include <mutex>
#include <optional>
#include <thread>

namespace MWLua
{
    class LuaManager;

    class Worker
    {
    public:
        explicit Worker(LuaManager& manager);

        ~Worker();

        void allowUpdate(Misc::FrameClock::time_point statsEpoch, unsigned int frameNumber, Misc::FrameStats& stats);

        void finishUpdate(Misc::FrameClock::time_point statsEpoch, unsigned int frameNumber, Misc::FrameStats& stats);

        // Starts incremental garbage collection on the Lua thread. Runs in small
        // steps until finishGc() is called or a full collection cycle completes.
        // Without a Lua thread, does the configured amount of GC synchronously
        // instead.
        void gc();

        // Stops garbage collection, waiting only for the step in flight. Must be
        // called before the main thread touches the Lua state again.
        void finishGc();

        void join();

    private:
        struct UpdateRequest
        {
            Misc::FrameClock::time_point mStatsEpoch;
            unsigned mFrameNumber;
            Misc::FrameStats* mStats;
        };

        void update(Misc::FrameClock::time_point statsEpoch, unsigned frameNumber, Misc::FrameStats& stats);

        void run() noexcept;

        LuaManager& mManager;
        std::mutex mMutex;
        std::condition_variable mCV;
        std::optional<UpdateRequest> mUpdateRequest;
        bool mGcRequest = false;
        bool mGcInProgress = false;
        bool mJoinRequest = false;
        std::optional<std::thread> mThread;
    };
}

#endif // OPENMW_MWLUA_LUAWORKER_H
