#ifndef OPENMW_ESM_JOURNALENTRY_H
#define OPENMW_ESM_JOURNALENTRY_H

#include <string>

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    // format 0, saved games only

    struct JournalEntry
    {
        enum Type
        {
            Type_Journal = 0,
            Type_Topic = 1,
            Type_Quest = 2
        };

        int mType;
        std::string mTopic;
        std::string mInfo;
        std::string mText;
        std::string mActorName; // Could also be Actor ID to allow switching of localisation, but since mText is plaintext anyway...
        int mDay; // time stamp
        int mMonth;
        int mDayOfMonth;

        void load (ESMReader &esm);
        void save (ESMWriter &esm) const;
    };
}

#endif
