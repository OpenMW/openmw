#include "regionsoundselector.hpp"

#include <components/fallback/fallback.hpp>
#include <components/misc/rng.hpp>

#include <algorithm>
#include <numeric>

#include "../mwbase/world.hpp"
#include "../mwworld/esmstore.hpp"

namespace MWSound
{
    namespace
    {
        int addChance(int result, const ESM::Region::SoundRef &v)
        {
            return result + v.mChance;
        }
    }

    RegionSoundSelector::RegionSoundSelector()
    : mMinTimeBetweenSounds(Fallback::Map::getFloat("Weather_Minimum_Time_Between_Environmental_Sounds"))
    , mMaxTimeBetweenSounds(Fallback::Map::getFloat("Weather_Maximum_Time_Between_Environmental_Sounds"))
    {}

    std::optional<std::string> RegionSoundSelector::getNextRandom(float duration, const std::string& regionName,
                                                                    const MWBase::World& world)
    {
        mTimePassed += duration;

        if (mTimePassed < mTimeToNextEnvSound)
            return {};

        const float a = Misc::Rng::rollClosedProbability();
        mTimeToNextEnvSound = mMinTimeBetweenSounds + (mMaxTimeBetweenSounds - mMinTimeBetweenSounds) * a;
        mTimePassed = 0;

        if (mLastRegionName != regionName)
        {
            mLastRegionName = regionName;
            mSumChance = 0;
        }

        const ESM::Region* const region = world.getStore().get<ESM::Region>().search(mLastRegionName);

        if (region == nullptr)
            return {};

        if (mSumChance == 0)
        {
            mSumChance = std::accumulate(region->mSoundList.begin(), region->mSoundList.end(), 0, addChance);
            if (mSumChance == 0)
                return {};
        }

        const int r = Misc::Rng::rollDice(std::max(mSumChance, 100));
        int pos = 0;

        const auto isSelected = [&] (const ESM::Region::SoundRef& sound)
        {
            if (r - pos < sound.mChance)
                return true;
            pos += sound.mChance;
            return false;
        };

        const auto it = std::find_if(region->mSoundList.begin(), region->mSoundList.end(), isSelected);

        if (it == region->mSoundList.end())
            return {};

        return it->mSound;
    }
}
