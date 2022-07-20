#ifndef GAME_MWDIALOGUE_JOURNALENTRY_H
#define GAME_MWDIALOGUE_JOURNALENTRY_H

#include <string>

namespace ESM
{
    struct JournalEntry;
}

namespace MWWorld
{
    class Ptr;
}

namespace MWDialogue
{
    /// \brief Basic quest/dialogue/topic entry
    struct Entry
    {
        std::string mInfoId;
        std::string mText;
        std::string mActorName; // optional

        Entry() = default;

        /// actor is optional
        Entry (const std::string& topic, const std::string& infoId, const MWWorld::Ptr& actor);

        Entry (const ESM::JournalEntry& record);

        std::string getText() const;

        void write (ESM::JournalEntry& entry) const;
    };

    /// \brief A dialogue entry
    ///
    /// Same as entry, but store TopicID
    struct JournalEntry : public Entry
    {
        std::string mTopic;

        JournalEntry() = default;

        JournalEntry (const std::string& topic, const std::string& infoId, const MWWorld::Ptr& actor);

        JournalEntry (const ESM::JournalEntry& record);

        void write (ESM::JournalEntry& entry) const;

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
            int day, int month, int dayOfMonth, const MWWorld::Ptr& actor);

        StampedJournalEntry (const ESM::JournalEntry& record);

        void write (ESM::JournalEntry& entry) const;

        static StampedJournalEntry makeFromQuest (const std::string& topic, int index, const MWWorld::Ptr& actor);
    };
}

#endif
