#ifndef OPENMW_ESM_WEATHERSTATE_H
#define OPENMW_ESM_WEATHERSTATE_H

#include <map>
#include <string>
#include <vector>
#include <components/esm/refid.hpp>

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    struct RegionWeatherState
    {
        int mWeather;
        std::vector<char> mChances;
    };

    struct WeatherState
    {
        ESM::RefId mCurrentRegion;
        float mTimePassed;
        bool mFastForward;
        float mWeatherUpdateTime;
        float mTransitionFactor;
        int mCurrentWeather;
        int mNextWeather;
        int mQueuedWeather;
        std::map<ESM::RefId, RegionWeatherState> mRegions;

        void load(ESMReader& esm);
        void save(ESMWriter& esm) const;
    };
}

#endif
