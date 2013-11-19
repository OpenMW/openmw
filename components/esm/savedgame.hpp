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
        std::string mPlayerClass;
        std::string mPlayerCell;
        TimeStamp mInGameTime;
        float mTimePlayed;

        /// \todo add field for screenshot

        void load (ESMReader &esm);
        void save (ESMWriter &esm) const;
    };
}

#endif
