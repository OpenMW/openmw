#ifndef GAME_SOUND_REGIONSOUNDSELECTOR_H
#define GAME_SOUND_REGIONSOUNDSELECTOR_H

#include <components/esm/refid.hpp>

namespace MWSound
{
    class RegionSoundSelector
    {
    public:
        ESM::RefId getNextRandom(float duration, const ESM::RefId& regionName);

        RegionSoundSelector();

    private:
        float mTimeToNextEnvSound = 0.0f;
        float mTimePassed = 0.0;
        float mMinTimeBetweenSounds;
        float mMaxTimeBetweenSounds;
    };
}

#endif
