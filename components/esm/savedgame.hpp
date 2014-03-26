#ifndef OPENMW_ESM_SAVEDGAME_H
#define OPENMW_ESM_SAVEDGAME_H

#include <vector>
#include <string>

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    // format 0, saved games only

    struct SavedGame
    {
        static unsigned int sRecordId;

        struct TimeStamp
        {
            float mGameHour;
            int mDay;
            int mMonth;
            int mYear;
        };

        std::vector<std::string> mContentFiles;
        std::string mPlayerName;
        int mPlayerLevel;
        // The (translated) name of the player class. So it will be displayed in the MW language
        // the savegame was made in, not the currently running language of MW.
        // However, savegames from a different MW language are not compatible anyway.
        // And if only the ID was stored here, we would need to
        // peek into the savegame to look for a class record in case it is a custom class.
        std::string mPlayerClassName;
        std::string mPlayerCell;
        TimeStamp mInGameTime;
        double mTimePlayed;
        std::string mDescription;
        std::vector<char> mScreenshot; // raw jpg-encoded data

        void load (ESMReader &esm);
        void save (ESMWriter &esm) const;
    };
}

#endif
