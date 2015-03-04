#include "loadsndg.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int SoundGenerator::sRecordId = REC_SNDG;

    void SoundGenerator::load(ESMReader &esm)
    {
        bool hasData = false;
        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            uint32_t name = esm.retSubName().val;
            switch (name)
            {
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
            }
        }
        if (!hasData)
            esm.fail("Missing DATA");
    }
    void SoundGenerator::save(ESMWriter &esm) const
    {
        esm.writeHNT("DATA", mType, 4);
        esm.writeHNOCString("CNAM", mCreature);
        esm.writeHNOCString("SNAM", mSound);
    }

    void SoundGenerator::blank()
    {
        mType = LeftFoot;
        mCreature.clear();
        mSound.clear();
    }
}
