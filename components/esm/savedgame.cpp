
#include "savedgame.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

unsigned int ESM::SavedGame::sRecordId = ESM::REC_SAVE;

void ESM::SavedGame::load (ESMReader &esm)
{
    mPlayerName = esm.getHNString("PNAM");
    esm.getHNOT (mPlayerLevel, "PLEV");
    mPlayerClass = esm.getHNString("PCLA");
    mPlayerCell = esm.getHNString("PCEL");
    esm.getHNT (mInGameTime, "TSTM", 16);
    esm.getHNT (mTimePlayed, "TIME");
    mDescription = esm.getHNString ("DESC");

    while (esm.isNextSub ("DEPE"))
        mContentFiles.push_back (esm.getHString());
}

void ESM::SavedGame::save (ESMWriter &esm) const
{
    esm.writeHNCString ("PNAM", mPlayerName);
    esm.writeHNT ("PLEV", mPlayerLevel);
    esm.writeHNCString ("PCLA", mPlayerClass);
    esm.writeHNCString ("PCEL", mPlayerCell);
    esm.writeHNT ("TSTM", mInGameTime, 16);
    esm.writeHNT ("TIME", mTimePlayed);
    esm.writeHNCString ("DESC", mDescription);

    for (std::vector<std::string>::const_iterator iter (mContentFiles.begin());
         iter!=mContentFiles.end(); ++iter)
         esm.writeHNCString (*iter, "DEPE");

}
