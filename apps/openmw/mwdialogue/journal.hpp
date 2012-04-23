#ifndef GAME_MMDIALOG_JOURNAL_H
#define GAME_MWDIALOG_JOURNAL_H

#include <string>
#include <deque>
#include <map>

#include "journalentry.hpp"
#include "quest.hpp"

namespace MWDialogue
{
    /// \brief The player's journal
    class Journal
    {
        public:

            typedef std::deque<StampedJournalEntry> TEntryContainer;
            typedef TEntryContainer::const_iterator TEntryIter;
            typedef std::map<std::string, Quest> TQuestContainer; // topc, quest
            typedef TQuestContainer::const_iterator TQuestIter;
            typedef std::map<std::string, Topic> TTopicContainer; // topic-id, topic-content
            typedef TTopicContainer::const_iterator TTopicIter;

        private:

            TEntryContainer mJournal;
            TQuestContainer mQuests;
            TTopicContainer mTopics;

            Quest& getQuest (const std::string& id);

        public:

            Journal();

            void addEntry (const std::string& id, int index);
            ///< Add a journal entry.

            void setJournalIndex (const std::string& id, int index);
            ///< Set the journal index without adding an entry.

            int getJournalIndex (const std::string& id) const;
            ///< Get the journal index.

            void addTopic (const std::string& topicId, const std::string& infoId);

            TEntryIter begin() const;
            ///< Iterator pointing to the begin of the main journal.
            ///
            /// \note Iterators to main journal entries will never become invalid.

            TEntryIter end() const;
            ///< Iterator pointing past the end of the main journal.

            TQuestIter questBegin() const;
            ///< Iterator pointing to the first quest (sorted by topic ID)

            TQuestIter questEnd() const;
            ///< Iterator pointing past the last quest.

            TTopicIter topicBegin() const;
            ///< Iterator pointing to the first topic (sorted by topic ID)
            ///
            /// \note The topic ID is identical with the user-visible topic string.

            TTopicIter topicEnd() const;
            ///< Iterator pointing past the last topic.
    };
}

#endif
