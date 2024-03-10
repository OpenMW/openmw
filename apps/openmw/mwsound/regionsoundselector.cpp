#include "regionsoundselector.hpp"

#include <components/esm3/loadregn.hpp>
#include <components/fallback/fallback.hpp>
#include <components/misc/rng.hpp>

#include "../mwbase/environment.hpp"
#include "../mwworld/esmstore.hpp"

namespace MWSound
{
    RegionSoundSelector::RegionSoundSelector()
        : mMinTimeBetweenSounds(Fallback::Map::getFloat("Weather_Minimum_Time_Between_Environmental_Sounds"))
        , mMaxTimeBetweenSounds(Fallback::Map::getFloat("Weather_Maximum_Time_Between_Environmental_Sounds"))
    {
    }

    ESM::RefId RegionSoundSelector::getNextRandom(float duration, const ESM::RefId& regionName)
    {
        mTimePassed += duration;

        if (mTimePassed < mTimeToNextEnvSound)
            return {};

        const float a = Misc::Rng::rollClosedProbability();
        mTimeToNextEnvSound = mMinTimeBetweenSounds + (mMaxTimeBetweenSounds - mMinTimeBetweenSounds) * a;
        mTimePassed = 0;

        const ESM::Region* const region
            = MWBase::Environment::get().getESMStore()->get<ESM::Region>().search(regionName);

        if (region == nullptr)
            return {};

        for (const ESM::Region::SoundRef& sound : region->mSoundList)
        {
            if (Misc::Rng::roll0to99() < sound.mChance)
                return sound.mSound;
        }
        return {};
    }
}
