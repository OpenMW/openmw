#ifndef GAME_SOUND_WARMQUEUE_H
#define GAME_SOUND_WARMQUEUE_H

#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <unordered_set>

#include <components/vfs/pathutil.hpp>

namespace VFS
{
    class Manager;
}

namespace MWSound
{
    class HeadCache;

    // Warms likely sounds off the frame thread.
    class WarmQueue
    {
    public:
        WarmQueue(const VFS::Manager& vfs, HeadCache& cache);
        ~WarmQueue();

        void enqueue(VFS::Path::Normalized path);

    private:
        void run() noexcept;

        const VFS::Manager& mVfs;
        HeadCache& mCache;
        std::mutex mMutex;
        std::condition_variable mCV;
        std::deque<VFS::Path::Normalized> mQueue;
        std::unordered_set<VFS::Path::Normalized, VFS::Path::Hash> mQueued;
        bool mQuit = false;
        std::thread mThread;
    };
}

#endif
