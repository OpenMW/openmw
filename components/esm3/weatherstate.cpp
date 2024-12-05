#include "weatherstate.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{
    namespace
    {
        constexpr NAME currentRegionRecord = "CREG";
        constexpr NAME timePassedRecord = "TMPS";
        constexpr NAME fastForwardRecord = "FAST";
        constexpr NAME weatherUpdateTimeRecord = "WUPD";
        constexpr NAME transitionFactorRecord = "TRFC";
        constexpr NAME currentWeatherRecord = "CWTH";
        constexpr NAME nextWeatherRecord = "NWTH";
        constexpr NAME queuedWeatherRecord = "QWTH";
        constexpr NAME regionNameRecord = "RGNN";
        constexpr NAME regionWeatherRecord = "RGNW";
        constexpr NAME regionChanceRecord = "RGNC";
    }
}

namespace ESM
{
    void WeatherState::load(ESMReader& esm)
    {
        mCurrentRegion = esm.getHNRefId(currentRegionRecord);
        esm.getHNT(mTimePassed, timePassedRecord);
        esm.getHNT(mFastForward, fastForwardRecord);
        esm.getHNT(mWeatherUpdateTime, weatherUpdateTimeRecord);
        esm.getHNT(mTransitionFactor, transitionFactorRecord);
        esm.getHNT(mCurrentWeather, currentWeatherRecord);
        esm.getHNT(mNextWeather, nextWeatherRecord);
        esm.getHNT(mQueuedWeather, queuedWeatherRecord);

        while (esm.isNextSub(regionNameRecord))
        {
            ESM::RefId regionID = esm.getRefId();
            RegionWeatherState region;
            esm.getHNT(region.mWeather, regionWeatherRecord);
            while (esm.isNextSub(regionChanceRecord))
            {
                uint8_t chance;
                esm.getHT(chance);
                region.mChances.push_back(chance);
            }

            mRegions.insert(std::make_pair(regionID, region));
        }
    }

    void WeatherState::save(ESMWriter& esm) const
    {
        esm.writeHNCRefId(currentRegionRecord, mCurrentRegion);
        esm.writeHNT(timePassedRecord, mTimePassed);
        esm.writeHNT(fastForwardRecord, mFastForward);
        esm.writeHNT(weatherUpdateTimeRecord, mWeatherUpdateTime);
        esm.writeHNT(transitionFactorRecord, mTransitionFactor);
        esm.writeHNT(currentWeatherRecord, mCurrentWeather);
        esm.writeHNT(nextWeatherRecord, mNextWeather);
        esm.writeHNT(queuedWeatherRecord, mQueuedWeather);

        auto it = mRegions.begin();
        for (; it != mRegions.end(); ++it)
        {
            esm.writeHNCRefId(regionNameRecord, it->first);
            esm.writeHNT(regionWeatherRecord, it->second.mWeather);
            for (const uint8_t& chance : it->second.mChances)
            {
                esm.writeHNT(regionChanceRecord, chance);
            }
        }
    }
}
