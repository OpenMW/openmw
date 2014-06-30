#ifndef GAME_MWDIALOG_TOPIC_H
#define GAME_MWDIALOG_TOPIC_H

#include <string>
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

            std::string mTopic;
            std::string mName;
            TEntryContainer mEntries;

        public:

            Topic();

            Topic (const std::string& topic);

            virtual ~Topic();

            virtual void addEntry (const JournalEntry& entry);
            ///< Add entry
            ///
            /// \note Redundant entries are ignored.

            void insertEntry (const ESM::JournalEntry& entry);
            ///< Add entry without checking for redundant entries or modifying the state of the
            /// topic otherwise

            std::string getTopic() const;

            virtual std::string getName() const;

            void removeLastAddedResponse (const std::string& actorName);

            TEntryIter begin() const;
            ///< Iterator pointing to the begin of the journal for this topic.

            TEntryIter end() const;
            ///< Iterator pointing past the end of the journal for this topic.
    };
}

#endif
