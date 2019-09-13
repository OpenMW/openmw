#ifndef OPENMW_ESM_WEATHERSTATE_H
#define OPENMW_ESM_WEATHERSTATE_H

#include <map>
#include <string>
#include <vector>

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    struct RegionWeatherState
    {
        int mWeather;
        std::vector<char> mChances;
    };

    enum WeatherType
    {
        Clear = 0,
        Cloudy = 1,
        Foggy = 2,
        Overcast = 3,
        Rain = 4,
        Thunder = 5,
        Ash = 6,
        Blight = 7,
        Snow = 8,
        Blizzard = 9
    };

    struct WeatherState
    {
        std::string mCurrentRegion;
        float mTimePassed;
        bool mFastForward;
        float mWeatherUpdateTime;
        float mTransitionFactor;
        int mCurrentWeather;
        int mNextWeather;
        int mQueuedWeather;
        std::map<std::string, RegionWeatherState> mRegions;

        void load(ESMReader& esm);
        void save(ESMWriter& esm) const;
    };
}

#endif
