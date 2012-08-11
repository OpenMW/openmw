#ifndef GAME_MWDIALOG_JOURNAL_H
#define GAME_MWDIALOG_JOURNAL_H

#include "../mwbase/journal.hpp"

#include "journalentry.hpp"
#include "quest.hpp"

namespace MWDialogue
{
    /// \brief The player's journal
    class Journal : public MWBase::Journal
    {
            TEntryContainer mJournal;
            TQuestContainer mQuests;
            TTopicContainer mTopics;

            Quest& getQuest (const std::string& id);

        public:

            Journal();

            virtual void addEntry (const std::string& id, int index);
            ///< Add a journal entry.

            virtual void setJournalIndex (const std::string& id, int index);
            ///< Set the journal index without adding an entry.

            virtual int getJournalIndex (const std::string& id) const;
            ///< Get the journal index.

            virtual void addTopic (const std::string& topicId, const std::string& infoId);

            virtual TEntryIter begin() const;
            ///< Iterator pointing to the begin of the main journal.
            ///
            /// \note Iterators to main journal entries will never become invalid.

            virtual TEntryIter end() const;
            ///< Iterator pointing past the end of the main journal.

            virtual TQuestIter questBegin() const;
            ///< Iterator pointing to the first quest (sorted by topic ID)

            virtual TQuestIter questEnd() const;
            ///< Iterator pointing past the last quest.

            virtual TTopicIter topicBegin() const;
            ///< Iterator pointing to the first topic (sorted by topic ID)
            ///
            /// \note The topic ID is identical with the user-visible topic string.

            virtual TTopicIter topicEnd() const;
            ///< Iterator pointing past the last topic.
    };
}

#endif
