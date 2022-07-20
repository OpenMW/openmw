#ifndef OPENMW_COMPONENTS_MISC_FRAMERATELIMITER_H
#define OPENMW_COMPONENTS_MISC_FRAMERATELIMITER_H

#include <chrono>
#include <thread>

namespace Misc
{
    class FrameRateLimiter
    {
        public:
            template <class Rep, class Ratio>
            explicit FrameRateLimiter(std::chrono::duration<Rep, Ratio> maxFrameDuration,
                                      std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now())
                : mMaxFrameDuration(std::chrono::duration_cast<std::chrono::steady_clock::duration>(maxFrameDuration))
                , mLastMeasurement(now)
                , mLastFrameDuration(0)
            {}

            std::chrono::steady_clock::duration getLastFrameDuration() const
            {
                return mLastFrameDuration;
            }

            void limit(std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now())
            {
                const auto passed = now - mLastMeasurement;
                const auto left = mMaxFrameDuration - passed;
                if (left > left.zero())
                {
                    std::this_thread::sleep_for(left);
                    mLastMeasurement = now + left;
                    mLastFrameDuration = mMaxFrameDuration;
                }
                else
                {
                    mLastMeasurement = now;
                    mLastFrameDuration = passed;
                }
            }

        private:
            std::chrono::steady_clock::duration mMaxFrameDuration;
            std::chrono::steady_clock::time_point mLastMeasurement;
            std::chrono::steady_clock::duration mLastFrameDuration;
    };

    inline Misc::FrameRateLimiter makeFrameRateLimiter(float frameRateLimit)
    {
        if (frameRateLimit > 0.0f)
            return Misc::FrameRateLimiter(std::chrono::duration<float>(1.0f / frameRateLimit));
        else
            return Misc::FrameRateLimiter(std::chrono::steady_clock::duration::zero());
    }
}

#endif
