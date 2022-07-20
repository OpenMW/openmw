#include "reporter.hpp"
#include "loadinglistener.hpp"

#include <condition_variable>
#include <cstddef>
#include <mutex>

namespace Loading
{
    void Reporter::addTotal(std::size_t value)
    {
        const std::lock_guard lock(mMutex);
        mTotal += value;
        mUpdated.notify_all();
    }

    void Reporter::addProgress(std::size_t value)
    {
        const std::lock_guard lock(mMutex);
        mProgress += value;
        mUpdated.notify_all();
    }

    void Reporter::complete()
    {
        const std::lock_guard lock(mMutex);
        mDone = true;
        mUpdated.notify_all();
    }

    void Reporter::wait(Listener& listener) const
    {
        std::unique_lock lock(mMutex);
        while (!mDone)
        {
            listener.setProgressRange(mTotal);
            listener.setProgress(mProgress);
            mUpdated.wait(lock);
        }
    }
}
