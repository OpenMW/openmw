#ifndef OPENMW_COMPONENTS_MISC_TIMER_H
#define OPENMW_COMPONENTS_MISC_TIMER_H

#include "rng.hpp"

namespace Misc
{
    enum class TimerStatus
    {
        Waiting,
        Elapsed,
    };

    class DeviatingPeriodicTimer
    {
        public:
            explicit DeviatingPeriodicTimer(float period, float deviation, float timeLeft)
                : mPeriod(period), mDeviation(deviation), mTimeLeft(timeLeft)
            {}

            TimerStatus update(float duration, Rng::Generator& prng)
            {
                if (mTimeLeft > 0)
                {
                    mTimeLeft -= duration;
                    return TimerStatus::Waiting;
                }

                mTimeLeft = Rng::deviate(mPeriod, mDeviation, prng);
                return TimerStatus::Elapsed;
            }

            void reset(float timeLeft) { mTimeLeft = timeLeft; }

        private:
            const float mPeriod;
            const float mDeviation;
            float mTimeLeft;
    };
}

#endif
