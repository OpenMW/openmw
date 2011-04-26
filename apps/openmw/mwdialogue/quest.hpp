#ifndef GAME_MMDIALOG_QUEST_H
#define GAME_MWDIALOG_QUEST_H

#include <string>
#include <vector>

#include "journalentry.hpp"

namespace MWWorld
{
    class World;
}

namespace MWDialogue
{
    /// \brief A quest in progress or a compelted quest
    class Quest
    {
        public:

            typedef std::vector<JournalEntry> TEntryContainer;
            typedef TEntryContainer::const_iterator TEntryIter;

        private:

            std::string mTopic;
            int mIndex;
            std::vector<JournalEntry> mEntries;
            bool mFinished;

        public:

            Quest();

            Quest (const std::string& topic);

            const std::string getName (const MWWorld::World& world) const;
            ///< May be an empty string

            int getIndex() const;

            void setIndex (int index, const MWWorld::World& world);
            ///< Calling this function with a non-existant index while throw an exception.

            bool isFinished() const;

            void addEntry (const JournalEntry& entry, const MWWorld::World& world);
            ///< Add entry and adjust index accordingly.
            ///
            /// \note Redundant entries are ignored, but the index is still adjusted.

            TEntryIter begin();
            ///< Iterator pointing to the begin of the journal for this quest.

            TEntryIter end();
            ///< Iterator pointing past the end of the journal for this quest.
    };
}

#endif
