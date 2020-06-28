#ifndef GAME_SOUND_REGIONSOUNDSELECTOR_H
#define GAME_SOUND_REGIONSOUNDSELECTOR_H

#include <boost/optional.hpp>

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
            boost::optional<std::string> getNextRandom(float duration, const std::string& regionName,
                                                       const MWBase::World& world);

        private:
            float mTimeToNextEnvSound = 0.0f;
            int mSumChance = 0;
            std::string mLastRegionName;
            float mTimePassed = 0.0;
    };
}

#endif
