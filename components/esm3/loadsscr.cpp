#include "loadsscr.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{
    void StartScript::load(ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;
        mRecordFlags = esm.getRecordFlags();

        bool hasData = false;
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
                case fourCC("DATA"):
                    mData = esm.getHString();
                    hasData = true;
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
        mRecordFlags = 0;
        mData.clear();
    }
}
