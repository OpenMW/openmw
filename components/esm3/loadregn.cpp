#include "loadregn.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "components/esm/defs.hpp"

namespace ESM
{
    unsigned int Region::sRecordId = REC_REGN;

    void Region::load(ESMReader &esm, bool &isDeleted)
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
                    mId = esm.getHString();
                    hasName = true;
                    break;
                case fourCC("FNAM"):
                    mName = esm.getHString();
                    break;
                case fourCC("WEAT"):
                {
                    esm.getSubHeader();
                    if (esm.getVer() == VER_12)
                    {
                        mData.mA = 0;
                        mData.mB = 0;
                        esm.getExact(&mData, sizeof(mData) - 2);
                    }
                    else if (esm.getVer() == VER_13)
                    {
                        // May include the additional two bytes (but not necessarily)
                        if (esm.getSubSize() == sizeof(mData))
                        {
                            esm.getT(mData);
                        }
                        else
                        {
                            mData.mA = 0;
                            mData.mB = 0;
                            esm.getExact(&mData, sizeof(mData)-2);
                        }
                    }
                    else
                    {
                        esm.fail("Don't know what to do in this version");
                    }
                    break;
                }
                case fourCC("BNAM"):
                    mSleepList = esm.getHString();
                    break;
                case fourCC("CNAM"):
                    esm.getHT(mMapColor);
                    break;
                case fourCC("SNAM"):
                {
                    esm.getSubHeader();
                    SoundRef sr;
                    sr.mSound.assign(esm.getString(32));
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

    void Region::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
            return;
        }

        esm.writeHNOCString("FNAM", mName);

        if (esm.getVersion() == VER_12)
            esm.writeHNT("WEAT", mData, sizeof(mData) - 2);
        else
            esm.writeHNT("WEAT", mData);

        esm.writeHNOCString("BNAM", mSleepList);

        esm.writeHNT("CNAM", mMapColor);
        for (std::vector<SoundRef>::const_iterator it = mSoundList.begin(); it != mSoundList.end(); ++it)
        {
            esm.startSubRecord("SNAM");
            esm.writeFixedSizeString(it->mSound, 32);
            esm.writeT(it->mChance);
            esm.endRecord("NPCO");
        }
    }

    void Region::blank()
    {
        mData.mClear = mData.mCloudy = mData.mFoggy = mData.mOvercast = mData.mRain =
            mData.mThunder = mData.mAsh = mData.mBlight = mData.mA = mData.mB = 0;

        mMapColor = 0;

        mName.clear();
        mSleepList.clear();
        mSoundList.clear();
    }
}
