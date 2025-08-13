#ifndef GAME_MWDIALOG_TOPIC_H
#define GAME_MWDIALOG_TOPIC_H

#include <string>
#include <string_view>
#include <vector>

#include "journalentry.hpp"

namespace ESM
{
    struct JournalEntry;
}

namespace MWDialogue
{
    /// \brief Collection of seen responses for a topic
    class Topic
    {
    public:
        typedef std::vector<Entry> TEntryContainer;
        typedef TEntryContainer::const_iterator TEntryIter;

    protected:
        ESM::RefId mTopic;
        std::string mName;
        TEntryContainer mEntries;

    public:
        Topic();

        Topic(const ESM::RefId& topic);

        virtual ~Topic() = default;

        virtual bool addEntry(const JournalEntry& entry);
        ///< Add entry
        ///
        /// \note Redundant entries are ignored.

        void insertEntry(const ESM::JournalEntry& entry);
        ///< Add entry without checking for redundant entries or modifying the state of the
        /// topic otherwise

        const ESM::RefId& getTopic() const;

        virtual std::string_view getName() const;

        void removeLastAddedResponse(std::string_view actorName);

        TEntryIter begin() const;
        ///< Iterator pointing to the begin of the journal for this topic.

        TEntryIter end() const;
        ///< Iterator pointing past the end of the journal for this topic.

        std::size_t size() const { return mEntries.size(); }

        const Entry& operator[](std::size_t i) const { return mEntries[i]; }
    };
}

#endif
