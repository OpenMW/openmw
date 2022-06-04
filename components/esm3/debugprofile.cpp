#include "debugprofile.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

void DebugProfile::load (ESMReader& esm, bool &isDeleted)
{
    isDeleted = false;
    mRecordFlags = esm.getRecordFlags();

    while (esm.hasMoreSubs())
    {
        esm.getSubName();
        switch (esm.retSubName().toInt())
        {
            case SREC_NAME:
                mId = esm.getHString();
                break;
            case fourCC("DESC"):
                mDescription = esm.getHString();
                break;
            case fourCC("SCRP"):
                mScriptText = esm.getHString();
                break;
            case fourCC("FLAG"):
                esm.getHT(mFlags);
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
}

void DebugProfile::save (ESMWriter& esm, bool isDeleted) const
{
    esm.writeHNCString ("NAME", mId);

    if (isDeleted)
    {
        esm.writeHNString("DELE", "", 3);
        return;
    }

    esm.writeHNCString ("DESC", mDescription);
    esm.writeHNCString ("SCRP", mScriptText);
    esm.writeHNT ("FLAG", mFlags);
}

void DebugProfile::blank()
{
    mDescription.clear();
    mScriptText.clear();
    mFlags = 0;
}

}
