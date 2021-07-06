#include "debugprofile.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

unsigned int ESM::DebugProfile::sRecordId = REC_DBGP;

void ESM::DebugProfile::load (ESMReader& esm, bool &isDeleted)
{
    isDeleted = false;

    while (esm.hasMoreSubs())
    {
        esm.getSubName();
        switch (esm.retSubName().intval)
        {
            case ESM::SREC_NAME:
                mId = esm.getHString();
                break;
            case ESM::FourCC<'D','E','S','C'>::value:
                mDescription = esm.getHString();
                break;
            case ESM::FourCC<'S','C','R','P'>::value:
                mScriptText = esm.getHString();
                break;
            case ESM::FourCC<'F','L','A','G'>::value:
                esm.getHT(mFlags);
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
}

void ESM::DebugProfile::save (ESMWriter& esm, bool isDeleted) const
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

void ESM::DebugProfile::blank()
{
    mDescription.clear();
    mScriptText.clear();
    mFlags = 0;
}
