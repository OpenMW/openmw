#include "loadsndg.hpp"

#include "components/esm/defs.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

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
                    esm.getHT(mType);
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
        esm.writeHNCRefId("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
            return;
        }

        esm.writeHNT("DATA", mType);
        esm.writeHNOCRefId("CNAM", mCreature);
        esm.writeHNOCRefId("SNAM", mSound);
    }

    void SoundGenerator::blank()
    {
        mRecordFlags = 0;
        mType = LeftFoot;
        mCreature = ESM::RefId();
        mSound = ESM::RefId();
    }
}
