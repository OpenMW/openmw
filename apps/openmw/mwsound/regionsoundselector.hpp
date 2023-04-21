#ifndef GAME_SOUND_REGIONSOUNDSELECTOR_H
#define GAME_SOUND_REGIONSOUNDSELECTOR_H

#include <components/esm/refid.hpp>
#include <optional>
#include <string>

namespace MWBase
{
    class World;
}

namespace MWSound
{
    class RegionSoundSelector
    {
    public:
        std::optional<ESM::RefId> getNextRandom(float duration, const ESM::RefId& regionName);

        RegionSoundSelector();

    private:
        float mTimeToNextEnvSound = 0.0f;
        int mSumChance = 0;
        ESM::RefId mLastRegionName;
        float mTimePassed = 0.0;
        float mMinTimeBetweenSounds;
        float mMaxTimeBetweenSounds;
    };
}

#endif
