#ifndef GAME_MMDIALOG_JOURNAL_H
#define GAME_MWDIALOG_JOURNAL_H

#include <string>
#include <deque>
#include <map>

#include "journalentry.hpp"
#include "quest.hpp"

namespace MWWorld
{
    struct Environment;
}

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

        private:

            MWWorld::Environment& mEnvironment;
            TEntryContainer mJournal;
            TQuestContainer mQuests;

            Quest& getQuest (const std::string& id);

        public:

            Journal (MWWorld::Environment& environment);

            void addEntry (const std::string& id, int index);
            ///< Add a journal entry.

            void setJournalIndex (const std::string& id, int index);
            ///< Set the journal index without adding an entry.

            int getJournalIndex (const std::string& id) const;
            ///< Get the journal index.

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
    };
}

#endif
