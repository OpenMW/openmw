#ifndef OPENMW_ESM_WEATHERSTATE_H
#define OPENMW_ESM_WEATHERSTATE_H

#include <string>

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    struct WeatherState
    {
        float mHour;
        float mWindSpeed;
        std::string mCurrentWeather;
        std::string mNextWeather;
        std::string mCurrentRegion;
        bool mFirstUpdate;
        float mRemainingTransitionTime;
        double mTimePassed;

        void load(ESMReader& esm);
        void save(ESMWriter& esm) const;
    };
}

#endif
