#ifndef COMPONENTS_LOADINGLISTENER_REPORTER_H
#define COMPONENTS_LOADINGLISTENER_REPORTER_H

#include <condition_variable>
#include <cstddef>
#include <mutex>

namespace Loading
{
    class Listener;

    class Reporter
    {
    public:
        void addTotal(std::size_t value);

        void addProgress(std::size_t value);

        void complete();

        void wait(Listener& listener) const;

    private:
        std::size_t mProgress = 0;
        std::size_t mTotal = 0;
        bool mDone = false;
        mutable std::mutex mMutex;
        mutable std::condition_variable mUpdated;
    };
}

#endif
