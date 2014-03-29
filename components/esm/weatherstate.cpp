#include "weatherstate.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace
{
    const char* hourRecord                    = "HOUR";
    const char* windSpeedRecord               = "WNSP";
    const char* currentWeatherRecord          = "CWTH";
    const char* nextWeatherRecord             = "NWTH";
    const char* currentRegionRecord           = "CREG";
    const char* firstUpdateRecord             = "FUPD";
    const char* remainingTransitionTimeRecord = "RTTM";
    const char* timePassedRecord              = "TMPS";
}

namespace ESM
{
    void WeatherState::load(ESMReader& esm)
    {
        // store values locally so that a failed load can't leave the state half set
        float newHour = 0.0;
        esm.getHNT(newHour, hourRecord);
        float newWindSpeed = 0.0;
        esm.getHNT(newWindSpeed, windSpeedRecord);
        std::string newCurrentWeather = esm.getHNString(currentWeatherRecord);
        std::string newNextWeather = esm.getHNString(nextWeatherRecord);
        std::string newCurrentRegion = esm.getHNString(currentRegionRecord);
        bool newFirstUpdate = false;
        esm.getHNT(newFirstUpdate, firstUpdateRecord);
        float newRemainingTransitionTime = 0.0;
        esm.getHNT(newRemainingTransitionTime, remainingTransitionTimeRecord);
        double newTimePassed = 0.0;
        esm.getHNT(newTimePassed, timePassedRecord);

        // swap values now that it is safe to do so
        mHour = newHour;
        mWindSpeed = newWindSpeed;
        mCurrentWeather.swap(newCurrentWeather);
        mNextWeather.swap(newNextWeather);
        mCurrentRegion.swap(newCurrentRegion);
        mFirstUpdate = newFirstUpdate;
        mRemainingTransitionTime = newRemainingTransitionTime;
        mTimePassed = newTimePassed;
    }

    void WeatherState::save(ESMWriter& esm) const
    {
        esm.writeHNT(hourRecord, mHour);
        esm.writeHNT(windSpeedRecord, mWindSpeed);
        esm.writeHNCString(currentWeatherRecord, mCurrentWeather.c_str());
        esm.writeHNCString(nextWeatherRecord, mNextWeather.c_str());
        esm.writeHNCString(currentRegionRecord, mCurrentRegion.c_str());
        esm.writeHNT(firstUpdateRecord, mFirstUpdate);
        esm.writeHNT(remainingTransitionTimeRecord, mRemainingTransitionTime);
        esm.writeHNT(timePassedRecord, mTimePassed);
    }
}
