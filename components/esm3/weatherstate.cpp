#include "weatherstate.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "loadregn.hpp"

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
        int currentWeather;
        esm.getHNT(currentWeather, currentWeatherRecord);
        mCurrentWeather = ESM::Weather::indexToRefId(currentWeather);
        int nextWeather;
        esm.getHNT(nextWeather, nextWeatherRecord);
        mNextWeather = ESM::Weather::indexToRefId(nextWeather);
        int queuedWeather;
        esm.getHNT(queuedWeather, queuedWeatherRecord);
        mQueuedWeather = ESM::Weather::indexToRefId(queuedWeather);

        while (esm.isNextSub(regionNameRecord))
        {
            ESM::RefId regionID = esm.getRefId();
            RegionWeatherState region;
            int weatherId;
            esm.getHNT(weatherId, regionWeatherRecord);
            region.mWeather = Weather::indexToRefId(weatherId);
            int index = 0;
            while (esm.isNextSub(regionChanceRecord))
            {
                uint8_t chance;
                esm.getHT(chance);
                ESM::RefId id = Weather::indexToRefId(index++);
                region.mChances.emplace(id, chance);
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
        esm.writeHNT(currentWeatherRecord, Weather::refIdToIndex(mCurrentWeather));
        esm.writeHNT(nextWeatherRecord, Weather::refIdToIndex(mNextWeather));
        esm.writeHNT(queuedWeatherRecord, Weather::refIdToIndex(mQueuedWeather));

        for (const auto& [region, weather] : mRegions)
        {
            esm.writeHNCRefId(regionNameRecord, region);
            esm.writeHNT(regionWeatherRecord, Weather::refIdToIndex(weather.mWeather));
            for (int i = 0; i < Weather::Length; ++i)
            {
                ESM::RefId id = Weather::indexToRefId(i);
                uint8_t chance = 0;
                const auto found = weather.mChances.find(id);
                if (found != weather.mChances.end())
                    chance = found->second;
                esm.writeHNT(regionChanceRecord, chance);
            }
        }
    }
}
