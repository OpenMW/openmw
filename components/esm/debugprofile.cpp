
#include "debugprofile.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

unsigned int ESM::DebugProfile::sRecordId = REC_DBGP;

void ESM::DebugProfile::load (ESMReader& esm)
{
    mDescription = esm.getHNString ("DESC");
    mScript = esm.getHNString ("SCRP");

    int default_ = 0;
    esm.getHNOT (default_, "DEFA");

    mDefault = default_!=0;

    int bypass = 0;
    esm.getHNOT (bypass, "BYNG");

    mBypassNewGame = bypass!=0;
}

void ESM::DebugProfile::save (ESMWriter& esm) const
{
    esm.writeHNCString ("DESC", mDescription);
    esm.writeHNCString ("SCRP", mScript);

    if (mDefault)
    {
        int default_ = 1;
        esm.writeHNT ("DEFA", default_);
    }

    if (mBypassNewGame)
    {
        int bypass = 1;
        esm.writeHNT ("BYNG", bypass);
    }
}

void ESM::DebugProfile::blank()
{
    mDescription.clear();
    mScript.clear();
    mDefault = false;
    mBypassNewGame = false;
}
