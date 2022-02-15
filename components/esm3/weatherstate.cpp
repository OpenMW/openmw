#include "weatherstate.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace
{
    constexpr ESM::NAME currentRegionRecord     = "CREG";
    constexpr ESM::NAME timePassedRecord        = "TMPS";
    constexpr ESM::NAME fastForwardRecord       = "FAST";
    constexpr ESM::NAME weatherUpdateTimeRecord = "WUPD";
    constexpr ESM::NAME transitionFactorRecord  = "TRFC";
    constexpr ESM::NAME currentWeatherRecord    = "CWTH";
    constexpr ESM::NAME nextWeatherRecord       = "NWTH";
    constexpr ESM::NAME queuedWeatherRecord     = "QWTH";
    constexpr ESM::NAME regionNameRecord        = "RGNN";
    constexpr ESM::NAME regionWeatherRecord     = "RGNW";
    constexpr ESM::NAME regionChanceRecord      = "RGNC";
}

namespace ESM
{
    void WeatherState::load(ESMReader& esm)
    {
        mCurrentRegion = esm.getHNString(currentRegionRecord);
        esm.getHNT(mTimePassed, timePassedRecord);
        esm.getHNT(mFastForward, fastForwardRecord);
        esm.getHNT(mWeatherUpdateTime, weatherUpdateTimeRecord);
        esm.getHNT(mTransitionFactor, transitionFactorRecord);
        esm.getHNT(mCurrentWeather, currentWeatherRecord);
        esm.getHNT(mNextWeather, nextWeatherRecord);
        esm.getHNT(mQueuedWeather, queuedWeatherRecord);

        while (esm.isNextSub(regionNameRecord))
        {
            std::string regionID = esm.getHString();
            RegionWeatherState region;
            esm.getHNT(region.mWeather, regionWeatherRecord);
            while (esm.isNextSub(regionChanceRecord))
            {
                char chance;
                esm.getHT(chance);
                region.mChances.push_back(chance);
            }

            mRegions.insert(std::make_pair(regionID, region));
        }
    }

    void WeatherState::save(ESMWriter& esm) const
    {
        esm.writeHNCString(currentRegionRecord, mCurrentRegion);
        esm.writeHNT(timePassedRecord, mTimePassed);
        esm.writeHNT(fastForwardRecord, mFastForward);
        esm.writeHNT(weatherUpdateTimeRecord, mWeatherUpdateTime);
        esm.writeHNT(transitionFactorRecord, mTransitionFactor);
        esm.writeHNT(currentWeatherRecord, mCurrentWeather);
        esm.writeHNT(nextWeatherRecord, mNextWeather);
        esm.writeHNT(queuedWeatherRecord, mQueuedWeather);

        std::map<std::string, RegionWeatherState>::const_iterator it = mRegions.begin();
        for(; it != mRegions.end(); ++it)
        {
            esm.writeHNCString(regionNameRecord, it->first.c_str());
            esm.writeHNT(regionWeatherRecord, it->second.mWeather);
            for(size_t i = 0; i < it->second.mChances.size(); ++i)
            {
                esm.writeHNT(regionChanceRecord, it->second.mChances[i]);
            }
        }
    }
}
