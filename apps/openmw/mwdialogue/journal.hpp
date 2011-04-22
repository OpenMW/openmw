#ifndef GAME_MMDIALOG_JOURNAL_H
#define GAME_MWDIALOG_JOURNAL_H

#include <string>
#include <deque>

#include "journalentry.hpp"

namespace MWWorld
{
    struct Environment;
}

namespace MWDialogue
{
    class Journal
    {
        public:

            typedef std::deque<StampedJournalEntry> TEntryContainer;
            typedef TEntryContainer::const_iterator TEntryIter;

        private:

            MWWorld::Environment& mEnvironment;
            TEntryContainer mJournal;

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
    };
}

#endif
