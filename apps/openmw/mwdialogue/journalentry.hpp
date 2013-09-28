#ifndef GAME_MWDIALOGUE_JOURNALENTRY_H
#define GAME_MWDIALOGUE_JOURNALENTRY_H

#include <string>

namespace ESMS
{
    struct ESMStore;
}

namespace MWDialogue
{
    /// \brief A quest or dialogue entry
    struct JournalEntry
    {
        std::string mTopic;
        std::string mInfoId;

        JournalEntry();

        JournalEntry (const std::string& topic, const std::string& infoId);

        std::string getText (const ESMS::ESMStore& store) const;

        static JournalEntry makeFromQuest (const std::string& topic, int index);

        static std::string idFromIndex (const std::string& topic, int index);
    };

    /// \biref A quest entry with a timestamp.
    struct StampedJournalEntry : public JournalEntry
    {
        int mDay;
        int mMonth;
        int mDayOfMonth;

        StampedJournalEntry();

        StampedJournalEntry (const std::string& topic, const std::string& infoId,
            int day, int month, int dayOfMonth);

        static StampedJournalEntry makeFromQuest (const std::string& topic, int index);
    };
}

#endif
