#ifndef GAME_MWDIALOGUE_JOURNALENTRY_H
#define GAME_MWDIALOGUE_JOURNALENTRY_H

#include <components/esm/refid.hpp>
#include <string>
#include <string_view>

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
        ESM::RefId mInfoId;
        std::string mText;
        std::string mActorName; // optional

        Entry() = default;

        /// actor is optional
        Entry(const ESM::RefId& topic, const ESM::RefId& infoId, const MWWorld::Ptr& actor);

        Entry(const ESM::JournalEntry& record);

        const std::string& getText() const;

        void write(ESM::JournalEntry& entry) const;
    };

    /// \brief A dialogue entry
    ///
    /// Same as entry, but store TopicID
    struct JournalEntry : public Entry
    {
        ESM::RefId mTopic;

        JournalEntry() = default;

        JournalEntry(const ESM::RefId& topic, const ESM::RefId& infoId, const MWWorld::Ptr& actor);

        JournalEntry(const ESM::JournalEntry& record);

        void write(ESM::JournalEntry& entry) const;

        static JournalEntry makeFromQuest(const ESM::RefId& topic, int index);

        static const ESM::RefId& idFromIndex(const ESM::RefId& topic, int index);
    };

    /// \brief A quest entry with a timestamp.
    struct StampedJournalEntry : public JournalEntry
    {
        int mDay;
        int mMonth;
        int mDayOfMonth;

        StampedJournalEntry();

        StampedJournalEntry(const ESM::RefId& topic, const ESM::RefId& infoId, int day, int month, int dayOfMonth,
            const MWWorld::Ptr& actor);

        StampedJournalEntry(const ESM::JournalEntry& record);

        void write(ESM::JournalEntry& entry) const;

        static StampedJournalEntry makeFromQuest(const ESM::RefId& topic, int index, const MWWorld::Ptr& actor);
    };
}

#endif
