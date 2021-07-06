#include "loadsoun.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Sound::sRecordId = REC_SOUN;

    void Sound::load(ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;

        bool hasName = false;
        bool hasData = false;
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
                    mSound = esm.getHString();
                    break;
                case ESM::FourCC<'D','A','T','A'>::value:
                    esm.getHT(mData, 3);
                    hasData = true;
                    break;
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
        if (!hasData && !isDeleted)
            esm.fail("Missing DATA subrecord");
    }

    void Sound::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
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
    }
}
