
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
    esm.writeHNString ("PNAM", mPlayerName);
    esm.writeHNT ("PLEV", mPlayerLevel);
    esm.writeHNString ("PCLA", mPlayerClass);
    esm.writeHNString ("PCEL", mPlayerCell);
    esm.writeHNT ("TSTM", mInGameTime, 16);
    esm.writeHNT ("TIME", mTimePlayed);
    esm.writeHNString ("DESC", mDescription);

    for (std::vector<std::string>::const_iterator iter (mContentFiles.begin());
         iter!=mContentFiles.end(); ++iter)
         esm.writeHNCString ("DEPE", *iter);

}
