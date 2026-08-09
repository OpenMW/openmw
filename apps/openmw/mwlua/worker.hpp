#ifndef OPENMW_MWLUA_WORKER_H
#define OPENMW_MWLUA_WORKER_H

#include <osg/Timer>
#include <osg/ref_ptr>

#include <condition_variable>
#include <mutex>
#include <optional>
#include <thread>

namespace osg
{
    class Stats;
}

namespace MWLua
{
    class LuaManager;

    class Worker
    {
    public:
        explicit Worker(LuaManager& manager);

        ~Worker();

        void allowUpdate(osg::Timer_t frameStart, unsigned int frameNumber, osg::Stats& stats);

        void finishUpdate(osg::Timer_t frameStart, unsigned int frameNumber, osg::Stats& stats);

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
            osg::Timer_t mFrameStart;
            unsigned mFrameNumber;
            osg::ref_ptr<osg::Stats> mStats;
        };

        void update(osg::Timer_t frameStart, unsigned frameNumber, osg::Stats& stats);

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
