#ifndef GAME_MWDIALOGUE_JOURNALENTRY_H
#define GAME_MWDIALOGUE_JOURNALENTRY_H

#include <string>

namespace MWDialogue
{
    /// \brief Basic quest/dialogue/topic entry
    struct Entry
    {
        std::string mInfoId;
        std::string mText;

        Entry();

        Entry (const std::string& topic, const std::string& infoId);

        std::string getText() const;
    };

    /// \brief A dialogue entry
    ///
    /// Same as entry, but store TopicID
    struct JournalEntry : public Entry
    {
        std::string mTopic;

        JournalEntry();

        JournalEntry (const std::string& topic, const std::string& infoId);

        static JournalEntry makeFromQuest (const std::string& topic, int index);

        static std::string idFromIndex (const std::string& topic, int index);
    };

    /// \brief A quest entry with a timestamp.
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
