#include "debugprofile.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

unsigned int ESM::DebugProfile::sRecordId = REC_DBGP;

ESM::DebugProfile::DebugProfile()
    : mIsDeleted(false)
{}

void ESM::DebugProfile::load (ESMReader& esm)
{
    mIsDeleted = false;

    while (esm.hasMoreSubs())
    {
        esm.getSubName();
        uint32_t name = esm.retSubName().val;
        switch (name)
        {
            case ESM::FourCC<'N','A','M','E'>::value:
                mId = esm.getHString();
                break;
            case ESM::FourCC<'D','E','L','E'>::value:
                esm.skipHSub();
                mIsDeleted = true;
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
            default:
                esm.fail("Unknown subrecord");
                break;
        }
    }
}

void ESM::DebugProfile::save (ESMWriter& esm) const
{
    esm.writeHNCString ("NAME", mId);

    if (mIsDeleted)
    {
        esm.writeHNCString("DELE", "");
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
    mIsDeleted = false;
}
