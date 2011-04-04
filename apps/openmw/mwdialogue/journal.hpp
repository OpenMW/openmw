#ifndef GAME_MMDIALOG_JOURNAL_H
#define GAME_MWDIALOG_JOURNAL_H

#include <string>

namespace MWWorld
{
    struct Environment;
}

namespace MWDialogue
{
    class Journal
    {
            MWWorld::Environment& mEnvironment;

        public:

            Journal (MWWorld::Environment& environment);

            void addEntry (const std::string& id, int index);
            ///< Add a journal entry.

            void setJournalIndex (const std::string& id, int index);
            ///< Set the journal index without adding an entry.

            int getJournalIndex (const std::string& id) const;
            ///< Get the journal index.
    };
}

#endif
