#ifndef OPENMW_BARRIER_H
#define OPENMW_BARRIER_H

#include <condition_variable>
#include <mutex>

namespace Misc
{
    /// @brief Synchronize several threads
    class Barrier
    {
        public:
            /// @param count number of threads to wait on
            explicit Barrier(int count) : mThreadCount(count), mRendezvousCount(0), mGeneration(0)
            {}

            /// @brief stop execution of threads until count distinct threads reach this point
            /// @param func callable to be executed once after all threads have met
            template <class Callback>
            void wait(Callback&& func)
            {
                std::unique_lock lock(mMutex);

                ++mRendezvousCount;
                const int currentGeneration = mGeneration;
                if (mRendezvousCount == mThreadCount)
                {
                    ++mGeneration;
                    mRendezvousCount = 0;
                    func();
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
    };
}

#endif
