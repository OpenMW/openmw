#ifndef OPENMW_ESM_SAVEDGAME_H
#define OPENMW_ESM_SAVEDGAME_H

#include <vector>
#include <string>

#include "defs.hpp"

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    // format 0, saved games only

    struct SavedGame
    {
        static unsigned int sRecordId;

        static int sCurrentFormat;

        std::vector<std::string> mContentFiles;
        std::string mPlayerName;
        int mPlayerLevel;

        // ID of class
        std::string mPlayerClassId;
        // Name of the class. When using a custom class, the ID is not really meaningful prior
        // to loading the savegame, so the name is stored separately.
        std::string mPlayerClassName;

        std::string mPlayerCell;
        EpochTimeStamp mInGameTime;
        double mTimePlayed;
        std::string mDescription;
        std::vector<char> mScreenshot; // raw jpg-encoded data

        void load (ESMReader &esm);
        void save (ESMWriter &esm) const;
    };
}

#endif
