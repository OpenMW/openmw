#ifndef OPENMW_ESM_SAVEDGAME_H
#define OPENMW_ESM_SAVEDGAME_H

#include <string>
#include <vector>

#include "components/esm/defs.hpp"
#include "components/esm/refid.hpp"

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    // format 0, saved games only

    struct SavedGame
    {
        constexpr static RecNameInts sRecordId = REC_SAVE;

        std::vector<std::string> mContentFiles;
        std::string mPlayerName;
        int mPlayerLevel;

        // ID of class
        ESM::RefId mPlayerClassId;
        // Name of the class. When using a custom class, the ID is not really meaningful prior
        // to loading the savegame, so the name is stored separately.
        std::string mPlayerClassName;
        // Name of the cell. Note that this is not an ID.
        std::string mPlayerCellName;
        EpochTimeStamp mInGameTime;
        double mTimePlayed;
        std::string mDescription;
        std::vector<char> mScreenshot; // raw jpg-encoded data

        void load(ESMReader& esm);
        void save(ESMWriter& esm) const;
    };
}

#endif
