#include "loadregn.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include <components/esm/common.hpp>

namespace
{
    const static std::array<ESM::RefId, 10> sWeatherIds = {
        ESM::StringRefId("Clear"),
        ESM::StringRefId("Cloudy"),
        ESM::StringRefId("Foggy"),
        ESM::StringRefId("Overcast"),
        ESM::StringRefId("Rain"),
        ESM::StringRefId("Thunderstorm"),
        ESM::StringRefId("Ashstorm"),
        ESM::StringRefId("Blight"),
        ESM::StringRefId("Snow"),
        ESM::StringRefId("Blizzard"),
    };
}

namespace ESM
{
    namespace Weather
    {
        int refIdToIndex(ESM::RefId id)
        {
            for (size_t i = 0; i < sWeatherIds.size(); ++i)
            {
                if (id == sWeatherIds[i])
                    return static_cast<int>(i);
            }
            return -1;
        }

        ESM::RefId indexToRefId(int index)
        {
            if (index >= 0 && static_cast<size_t>(index) < sWeatherIds.size())
                return sWeatherIds[index];
            return {};
        }
    }

    void Region::load(ESMReader& esm, bool& isDeleted)
    {
        isDeleted = false;
        mRecordFlags = esm.getRecordFlags();

        bool hasName = false;
        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().toInt())
            {
                case SREC_NAME:
                    mId = esm.getRefId();
                    hasName = true;
                    break;
                case fourCC("FNAM"):
                    mName = esm.getHString();
                    break;
                case fourCC("WEAT"):
                {
                    esm.getSubHeader();
                    std::array<uint8_t, Weather::Length> probabilities;
                    // Cold weather not included before 1.3
                    if (esm.getSubSize() == Weather::Length)
                    {
                        esm.getT(probabilities);
                    }
                    else if (esm.getSubSize() == Weather::Length - 2)
                    {
                        probabilities.fill(0);
                        esm.getExact(&probabilities, esm.getSubSize());
                    }
                    else
                    {
                        esm.fail("Don't know what to do in this version");
                    }
                    for (int i = 0; i < Weather::Length; ++i)
                    {
                        if (probabilities[i] != 0)
                            mData.mProbabilities[Weather::indexToRefId(i)] = probabilities[i];
                    }
                    break;
                }
                case fourCC("BNAM"):
                    mSleepList = esm.getRefId();
                    break;
                case fourCC("CNAM"):
                    esm.getHT(mMapColor);
                    break;
                case fourCC("SNAM"):
                {
                    esm.getSubHeader();
                    SoundRef sr;
                    sr.mSound = esm.getMaybeFixedRefIdSize(32);
                    esm.getT(sr.mChance);
                    mSoundList.push_back(sr);
                    break;
                }
                case SREC_DELE:
                    esm.skipHSub();
                    isDeleted = true;
                    break;
                default:
                    esm.fail("Unknown subrecord");
                    break;
            }
        }

        if (!hasName)
            esm.fail("Missing NAME subrecord");
    }

    void Region::save(ESMWriter& esm, bool isDeleted) const
    {
        esm.writeHNCRefId("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
            return;
        }

        esm.writeHNOCString("FNAM", mName);

        std::array<uint8_t, Weather::Length> probabilities;
        probabilities.fill(0);
        for (const auto& [id, chance] : mData.mProbabilities)
        {
            const int index = Weather::refIdToIndex(id);
            if (index >= 0)
                probabilities[index] = chance;
        }
        if (esm.getVersion() == VER_120)
            esm.writeHNT("WEAT", probabilities, probabilities.size() - 2);
        else
            esm.writeHNT("WEAT", probabilities);

        esm.writeHNOCRefId("BNAM", mSleepList);

        esm.writeHNT("CNAM", mMapColor);
        for (std::vector<SoundRef>::const_iterator it = mSoundList.begin(); it != mSoundList.end(); ++it)
        {
            esm.startSubRecord("SNAM");
            esm.writeMaybeFixedSizeRefId(it->mSound, 32);
            esm.writeT(it->mChance);
            esm.endRecord("SNAM");
        }
    }

    void Region::blank()
    {
        mRecordFlags = 0;
        mData.mProbabilities.clear();

        mMapColor = 0;

        mName.clear();
        mSleepList = ESM::RefId();
        mSoundList.clear();
    }
}
