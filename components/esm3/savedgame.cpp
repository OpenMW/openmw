#include "savedgame.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <components/esm/defs.hpp>

namespace ESM
{

    int SavedGame::sCurrentFormat = 22;

    void SavedGame::load(ESMReader& esm)
    {
        mPlayerName = esm.getHNString("PLNA");
        esm.getHNOT(mPlayerLevel, "PLLE");

        mPlayerClassId = ESM::RefId::stringRefId(esm.getHNOString("PLCL"));
        mPlayerClassName = esm.getHNOString("PLCN");

        mPlayerCell = ESM::RefId::stringRefId(esm.getHNString("PLCE"));
        esm.getHNTSized<16>(mInGameTime, "TSTM");
        esm.getHNT(mTimePlayed, "TIME");
        mDescription = esm.getHNString("DESC");

        while (esm.isNextSub("DEPE"))
            mContentFiles.push_back(esm.getHString());

        esm.getSubNameIs("SCRN");
        esm.getSubHeader();
        mScreenshot.resize(esm.getSubSize());
        esm.getExact(mScreenshot.data(), mScreenshot.size());
    }

    void SavedGame::save(ESMWriter& esm) const
    {
        esm.writeHNString("PLNA", mPlayerName);
        esm.writeHNT("PLLE", mPlayerLevel);

        if (!mPlayerClassId.empty())
            esm.writeHNString("PLCL", mPlayerClassId.getRefIdString());
        else
            esm.writeHNString("PLCN", mPlayerClassName);

        esm.writeHNString("PLCE", mPlayerCell.getRefIdString());
        esm.writeHNT("TSTM", mInGameTime, 16);
        esm.writeHNT("TIME", mTimePlayed);
        esm.writeHNString("DESC", mDescription);

        for (std::vector<std::string>::const_iterator iter(mContentFiles.begin()); iter != mContentFiles.end(); ++iter)
            esm.writeHNString("DEPE", *iter);

        esm.startSubRecord("SCRN");
        esm.write(mScreenshot.data(), mScreenshot.size());
        esm.endRecord("SCRN");
    }

}
