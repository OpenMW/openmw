#include "loadsoun.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Sound::sRecordId = REC_SOUN;

    Sound::Sound()
        : mIsDeleted(false)
    {}

    void Sound::load(ESMReader &esm)
    {
        mIsDeleted = false;

        bool hasName = false;
        bool hasData = false;
        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            uint32_t name = esm.retSubName().val;
            switch (name)
            {
                case ESM::FourCC<'N','A','M','E'>::value:
                    mId = esm.getHString();
                    hasName = true;
                    break;
                case ESM::FourCC<'D','E','L','E'>::value:
                    esm.skipHSub();
                    mIsDeleted = true;
                    break;
                case ESM::FourCC<'F','N','A','M'>::value:
                    mSound = esm.getHString();
                    break;
                case ESM::FourCC<'D','A','T','A'>::value:
                    esm.getHT(mData, 3);
                    hasData = true;
                    break;
                default:
                    esm.fail("Unknown subrecord");
                    break;
            }
        }

        if (!hasName)
            esm.fail("Missing NAME subrecord");
        if (!hasData && !mIsDeleted)
            esm.fail("Missing DATA subrecord");
    }

    void Sound::save(ESMWriter &esm) const
    {
        esm.writeHNCString("NAME", mId);

        if (mIsDeleted)
        {
            esm.writeHNCString("DELE", "");
            return;
        }

        esm.writeHNOCString("FNAM", mSound);
        esm.writeHNT("DATA", mData, 3);
    }

    void Sound::blank()
    {
        mSound.clear();

        mData.mVolume = 128;
        mData.mMinRange = 0;
        mData.mMaxRange = 255;
        
        mIsDeleted = false;
    }
}
