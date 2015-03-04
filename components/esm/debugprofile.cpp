
#include "debugprofile.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

unsigned int ESM::DebugProfile::sRecordId = REC_DBGP;

void ESM::DebugProfile::load (ESMReader& esm)
{
    mDescription = esm.getHNString ("DESC");
    mScriptText = esm.getHNString ("SCRP");
    esm.getHNT (mFlags, "FLAG");
}

void ESM::DebugProfile::save (ESMWriter& esm) const
{
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
