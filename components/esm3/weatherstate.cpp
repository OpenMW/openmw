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
        if (esm.getFormatVersion() <= ESM::MaxWeatherIndexFormatVersion)
        {
            int currentWeather;
            esm.getHNT(currentWeather, currentWeatherRecord);
            mCurrentWeather = ESM::Weather::indexToRefId(currentWeather);
            int nextWeather;
            esm.getHNT(nextWeather, nextWeatherRecord);
            mNextWeather = ESM::Weather::indexToRefId(nextWeather);
            int queuedWeather;
            esm.getHNT(queuedWeather, queuedWeatherRecord);
            mQueuedWeather = ESM::Weather::indexToRefId(queuedWeather);
        }
        else
        {
            mCurrentWeather = esm.getHNORefId(currentWeatherRecord);
            mNextWeather = esm.getHNORefId(nextWeatherRecord);
            mQueuedWeather = esm.getHNORefId(queuedWeatherRecord);
        }

        while (esm.isNextSub(regionNameRecord))
        {
            ESM::RefId regionID = esm.getRefId();
            RegionWeatherState region;
            if (esm.getFormatVersion() <= ESM::MaxWeatherIndexFormatVersion)
            {
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
            }
            else
            {
                region.mWeather = esm.getHNORefId(regionWeatherRecord);
                while (esm.isNextSub(regionChanceRecord))
                {
                    esm.getSubHeader();
                    uint8_t chance;
                    esm.getT(chance);
                    ESM::RefId id = esm.getRefId(esm.getSubSize() - sizeof(chance));
                    region.mChances.emplace(id, chance);
                }
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
        esm.writeHNORefId(currentWeatherRecord, mCurrentWeather);
        esm.writeHNORefId(nextWeatherRecord, mNextWeather);
        esm.writeHNORefId(queuedWeatherRecord, mQueuedWeather);

        for (const auto& [region, weather] : mRegions)
        {
            esm.writeHNCRefId(regionNameRecord, region);
            esm.writeHNCRefId(regionWeatherRecord, weather.mWeather);
            for (const auto& [id, chance] : weather.mChances)
            {
                if (chance == 0)
                    continue;
                esm.startSubRecord(regionChanceRecord);
                esm.writeT(chance);
                esm.writeHRefId(id);
                esm.endRecord(regionChanceRecord);
            }
        }
    }
}
