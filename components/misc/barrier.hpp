#ifndef OPENMW_BARRIER_H
#define OPENMW_BARRIER_H

#include <condition_variable>
#include <functional>
#include <mutex>

namespace Misc
{
    /// @brief Synchronize several threads
    class Barrier
    {
        public:
            using BarrierCallback = std::function<void(void)>;
            /// @param count number of threads to wait on
            /// @param func callable to be executed once after all threads have met
            Barrier(int count, BarrierCallback&& func) : mThreadCount(count), mRendezvousCount(0), mGeneration(0)
                                                       , mFunc(std::forward<BarrierCallback>(func))
            {}

            /// @brief stop execution of threads until count distinct threads reach this point
            void wait()
            {
                std::unique_lock<std::mutex> lock(mMutex);

                ++mRendezvousCount;
                const int currentGeneration = mGeneration;
                if (mRendezvousCount == mThreadCount)
                {
                    ++mGeneration;
                    mRendezvousCount = 0;
                    mFunc();
                    mRendezvous.notify_all();
                }
                else
                {
                    mRendezvous.wait(lock, [&]() { return mGeneration != currentGeneration; });
                }
            }

        private:
            int mThreadCount;
            int mRendezvousCount;
            int mGeneration;
            mutable std::mutex mMutex;
            std::condition_variable mRendezvous;
            BarrierCallback mFunc;
    };
}

#endif
