#ifndef OPENMW_MWLUA_WORKER_H
#define OPENMW_MWLUA_WORKER_H

#include <condition_variable>
#include <mutex>
#include <optional>
#include <thread>

namespace osgViewer
{
    class Viewer;
}

namespace MWLua
{
    class LuaManager;

    class Worker
    {
    public:
        explicit Worker(LuaManager& manager, osgViewer::Viewer& viewer);

        ~Worker();

        void allowUpdate();

        void finishUpdate();

        void join();

    private:
        void update();

        void run() noexcept;

        LuaManager& mManager;
        osgViewer::Viewer& mViewer;
        std::mutex mMutex;
        std::condition_variable mCV;
        bool mUpdateRequest = false;
        bool mJoinRequest = false;
        std::optional<std::thread> mThread;
    };
}

#endif // OPENMW_MWLUA_LUAWORKER_H
