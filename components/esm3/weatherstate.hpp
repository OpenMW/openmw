#ifndef OPENMW_ESM_WEATHERSTATE_H
#define OPENMW_ESM_WEATHERSTATE_H

#include <components/esm/refid.hpp>
#include <map>
#include <string>
#include <vector>

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    struct RegionWeatherState
    {
        int32_t mWeather;
        std::vector<uint8_t> mChances;
    };

    struct WeatherState
    {
        ESM::RefId mCurrentRegion;
        float mTimePassed;
        bool mFastForward;
        float mWeatherUpdateTime;
        float mTransitionFactor;
        int32_t mCurrentWeather;
        int32_t mNextWeather;
        int32_t mQueuedWeather;
        std::map<ESM::RefId, RegionWeatherState> mRegions;

        void load(ESMReader& esm);
        void save(ESMWriter& esm) const;
    };
}

#endif
