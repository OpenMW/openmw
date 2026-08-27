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

    void WarmQueue::enqueue(VFS::Path::Normalized path, bool urgent)
    {
        push(std::move(path), urgent, true);
    }

    void WarmQueue::enqueueStreamed(VFS::Path::Normalized path)
    {
        push(std::move(path), false, false);
    }

    void WarmQueue::push(VFS::Path::Normalized path, bool urgent, bool wholeFile)
    {
        // Allow head-only entries to upgrade.
        {
            std::lock_guard<std::mutex> lock(mMutex);
            const auto [it, inserted] = mQueued.try_emplace(path);
            if (!inserted)
            {
                it->second->mWholeFile |= wholeFile;
                if (urgent && !it->second->mUrgent)
                {
                    it->second->mUrgent = true;
                    mQueue.splice(mQueue.begin(), mQueue, it->second);
                }
                return;
            }
            if (urgent)
            {
                mQueue.push_front({ std::move(path), true, wholeFile });
                it->second = mQueue.begin();
            }
            else
            {
                mQueue.push_back({ std::move(path), false, wholeFile });
                it->second = std::prev(mQueue.end());
            }
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
            Item item = std::move(mQueue.front());
            mQueue.pop_front();
            mQueued.erase(item.mPath);
            lock.unlock();
            try
            {
                // Bulk warming never evicts.
                if (!item.mUrgent && mCache.full())
                    continue;
                // Streamed entries cache initialization ranges.
                if ((!item.mWholeFile || !mCache.warmWholeFile(item.mPath)) && !mCache.contains(item.mPath))
                {
                    FFmpegDecoder decoder(&mVfs, &mCache, /*recordHead=*/true);
                    decoder.open(item.mPath);
                }
            }
            catch (const std::exception& e)
            {
                Log(Debug::Verbose) << "Failed to warm sound " << item.mPath << ": " << e.what();
            }
        }
    }
}
