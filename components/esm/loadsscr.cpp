#include "loadsscr.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int StartScript::sRecordId = REC_SSCR;

    void StartScript::load(ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;

        bool hasData = false;
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
                case ESM::FourCC<'D','A','T','A'>::value:
                    mData = esm.getHString();
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
            esm.fail("Missing NAME");
        if (!hasData && !isDeleted)
            esm.fail("Missing DATA");
    }
    void StartScript::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);
        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
        }
        else
        {
            esm.writeHNString("DATA", mData);
        }
    }

    void StartScript::blank()
    {
        mData.clear();
    }
}
