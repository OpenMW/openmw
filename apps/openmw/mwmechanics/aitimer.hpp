#ifndef OPENMW_MECHANICS_AITIMER_H
#define OPENMW_MECHANICS_AITIMER_H

#include <components/misc/rng.hpp>
#include <components/misc/timer.hpp>

namespace MWMechanics
{
    constexpr float AI_REACTION_TIME = 0.25f;

    class AiReactionTimer
    {
        public:
            static constexpr float sDeviation = 0.1f;

            Misc::TimerStatus update(float duration) { return mImpl.update(duration); }

            void reset() { mImpl.reset(Misc::Rng::deviate(0, sDeviation)); }

        private:
            Misc::DeviatingPeriodicTimer mImpl {AI_REACTION_TIME, sDeviation,
                                                Misc::Rng::deviate(0, sDeviation)};
    };
}

#endif
