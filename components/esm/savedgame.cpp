#include "savedgame.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

unsigned int ESM::SavedGame::sRecordId = ESM::REC_SAVE;
int ESM::SavedGame::sCurrentFormat = 16;

void ESM::SavedGame::load (ESMReader &esm)
{
    mPlayerName = esm.getHNString("PLNA");
    esm.getHNOT (mPlayerLevel, "PLLE");

    mPlayerClassId = esm.getHNOString("PLCL");
    mPlayerClassName = esm.getHNOString("PLCN");

    mPlayerCell = esm.getHNString("PLCE");
    esm.getHNT (mInGameTime, "TSTM", 16);
    esm.getHNT (mTimePlayed, "TIME");
    mDescription = esm.getHNString ("DESC");

    while (esm.isNextSub ("DEPE"))
        mContentFiles.push_back (esm.getHString());

    esm.getSubNameIs("SCRN");
    esm.getSubHeader();
    mScreenshot.resize(esm.getSubSize());
    esm.getExact(mScreenshot.data(), mScreenshot.size());
}

void ESM::SavedGame::save (ESMWriter &esm) const
{
    esm.writeHNString ("PLNA", mPlayerName);
    esm.writeHNT ("PLLE", mPlayerLevel);

    if (!mPlayerClassId.empty())
        esm.writeHNString ("PLCL", mPlayerClassId);
    else
        esm.writeHNString ("PLCN", mPlayerClassName);

    esm.writeHNString ("PLCE", mPlayerCell);
    esm.writeHNT ("TSTM", mInGameTime, 16);
    esm.writeHNT ("TIME", mTimePlayed);
    esm.writeHNString ("DESC", mDescription);

    for (std::vector<std::string>::const_iterator iter (mContentFiles.begin());
         iter!=mContentFiles.end(); ++iter)
         esm.writeHNString ("DEPE", *iter);

    esm.startSubRecord("SCRN");
    esm.write(&mScreenshot[0], mScreenshot.size());
    esm.endRecord("SCRN");
}
