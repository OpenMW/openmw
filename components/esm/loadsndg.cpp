#include "loadsndg.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int SoundGenerator::sRecordId = REC_SNDG;

    SoundGenerator::SoundGenerator()
        : mType(LeftFoot),
          mIsDeleted(false)
    {}

    void SoundGenerator::load(ESMReader &esm)
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
                case ESM::FourCC<'D','A','T','A'>::value:
                    esm.getHT(mType, 4);
                    hasData = true;
                    break;
                case ESM::FourCC<'C','N','A','M'>::value:
                    mCreature = esm.getHString();
                    break;
                case ESM::FourCC<'S','N','A','M'>::value:
                    mSound = esm.getHString();
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
    void SoundGenerator::save(ESMWriter &esm) const
    {
        esm.writeHNCString("NAME", mId);
        esm.writeHNT("DATA", mType, 4);
        esm.writeHNOCString("CNAM", mCreature);
        esm.writeHNOCString("SNAM", mSound);
        
        if (mIsDeleted)
        {
            esm.writeHNCString("DELE", "");
        }
    }

    void SoundGenerator::blank()
    {
        mType = LeftFoot;
        mCreature.clear();
        mSound.clear();
        mIsDeleted = false;
    }
}
