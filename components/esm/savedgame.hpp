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
        std::string mPlayerClass; // this is the ID and not the name of the class
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
