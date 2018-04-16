#include "loadregn.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Region::sRecordId = REC_REGN;

    void Region::load(ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;

        bool hasName = false;
        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().intval)
            {
                case ESM::SREC_NAME:
                    mId = esm.getHString();
                    hasName = true;
                    break;
                case ESM::FourCC<'F','N','A','M'>::value:
                    mName = esm.getHString();
                    break;
                case ESM::FourCC<'W','E','A','T'>::value:
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
                            esm.getExact(&mData, sizeof(mData));
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
                case ESM::FourCC<'B','N','A','M'>::value:
                    mSleepList = esm.getHString();
                    break;
                case ESM::FourCC<'C','N','A','M'>::value:
                    esm.getHT(mMapColor);
                    break;
                case ESM::FourCC<'S','N','A','M'>::value:
                {
                    SoundRef sr;
                    esm.getHT(sr, 33);
                    mSoundList.push_back(sr);
                    break;
                }
                case ESM::SREC_DELE:
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
            esm.writeHNCString("DELE", "");
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
            esm.writeHNT<SoundRef>("SNAM", *it);
        }
    }

    void Region::blank()
    {
        mData.mClear = mData.mCloudy = mData.mFoggy = mData.mOvercast = mData.mRain =
            mData.mThunder = mData.mAsh, mData.mBlight = mData.mA = mData.mB = 0;

        mMapColor = 0;

        mName.clear();
        mSleepList.clear();
        mSoundList.clear();
    }
}
