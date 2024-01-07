#include "loadregn.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include <components/esm/common.hpp>

namespace ESM
{
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
                    // Cold weather not included before 1.3
                    if (esm.getSubSize() == mData.mProbabilities.size())
                    {
                        esm.getT(mData.mProbabilities);
                    }
                    else if (esm.getSubSize() == mData.mProbabilities.size() - 2)
                    {
                        mData.mProbabilities.fill(0);
                        esm.getExact(&mData.mProbabilities, esm.getSubSize());
                    }
                    else
                    {
                        esm.fail("Don't know what to do in this version");
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

        if (esm.getVersion() == VER_120)
            esm.writeHNT("WEAT", mData.mProbabilities, mData.mProbabilities.size() - 2);
        else
            esm.writeHNT("WEAT", mData.mProbabilities);

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
        mData.mProbabilities.fill(0);

        mMapColor = 0;

        mName.clear();
        mSleepList = ESM::RefId();
        mSoundList.clear();
    }
}
