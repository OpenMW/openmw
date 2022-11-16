#include "loadsndg.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <components/esm/defs.hpp>
#include <components/esm/esmcommon.hpp>
#include <components/esm/fourcc.hpp>

namespace ESM
{
    void SoundGenerator::load(ESMReader& esm, bool& isDeleted)
    {
        isDeleted = false;
        mRecordFlags = esm.getRecordFlags();

        bool hasName = false;
        bool hasData = false;
        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().toInt())
            {
                case SREC_NAME:
                    mId = esm.getRefId();
                    hasName = true;
                    break;
                case fourCC("DATA"):
                    esm.getHTSized<4>(mType);
                    hasData = true;
                    break;
                case fourCC("CNAM"):
                    mCreature = esm.getRefId();
                    break;
                case fourCC("SNAM"):
                    mSound = esm.getRefId();
                    break;
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
        if (!hasData && !isDeleted)
            esm.fail("Missing DATA subrecord");
    }
    void SoundGenerator::save(ESMWriter& esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId.getRefIdString());

        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
            return;
        }

        esm.writeHNT("DATA", mType, 4);
        esm.writeHNOCString("CNAM", mCreature.getRefIdString());
        esm.writeHNOCString("SNAM", mSound.getRefIdString());
    }

    void SoundGenerator::blank()
    {
        mRecordFlags = 0;
        mType = LeftFoot;
        mCreature = ESM::RefId::sEmpty;
        mSound = ESM::RefId::sEmpty;
    }
}
