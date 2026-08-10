#include "warmqueue.hpp"

#include <components/debug/debuglog.hpp>
#include <components/misc/thread.hpp>

#include "ffmpegdecoder.hpp"
#include "headcache.hpp"

namespace MWSound
{
    WarmQueue::WarmQueue(const VFS::Manager& vfs, HeadCache& cache)
        : mVfs(vfs)
        , mCache(cache)
        , mThread([this] { run(); })
    {
    }

    WarmQueue::~WarmQueue()
    {
        {
            std::lock_guard<std::mutex> lock(mMutex);
            mQuit = true;
        }
        mCV.notify_all();
        mThread.join();
    }

    void WarmQueue::enqueue(VFS::Path::Normalized path)
    {
        {
            std::lock_guard<std::mutex> lock(mMutex);
            if (!mQueued.emplace(path).second)
                return;
            mQueue.push_back(std::move(path));
        }
        mCV.notify_one();
    }

    void WarmQueue::run() noexcept
    {
        Misc::setCurrentThreadIdlePriority();
        while (true)
        {
            std::unique_lock<std::mutex> lock(mMutex);
            mCV.wait(lock, [&] { return mQuit || !mQueue.empty(); });
            if (mQuit)
                return;
            VFS::Path::Normalized path = std::move(mQueue.front());
            mQueue.pop_front();
            mQueued.erase(path);
            lock.unlock();
            try
            {
                // Bulk warming never evicts.
                if (mCache.full() || mCache.contains(path))
                    continue;
                // Cache stream initialization ranges.
                FFmpegDecoder decoder(&mVfs, &mCache);
                decoder.open(path);
            }
            catch (const std::exception& e)
            {
                Log(Debug::Verbose) << "Failed to warm sound " << path << ": " << e.what();
            }
        }
    }
}
