#include "debugprofile.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

unsigned int ESM::DebugProfile::sRecordId = REC_DBGP;

void ESM::DebugProfile::load (ESMReader& esm)
{
    mId = esm.getHNString ("NAME");
    mDescription = esm.getHNString ("DESC");
    mScriptText = esm.getHNString ("SCRP");
    esm.getHNT (mFlags, "FLAG");
}

void ESM::DebugProfile::save (ESMWriter& esm) const
{
    esm.writeHNCString ("NAME", mId);
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
