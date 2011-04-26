#ifndef GAME_MMDIALOG_TOPIC_H
#define GAME_MWDIALOG_TOPIC_H

#include <string>
#include <vector>

#include "journalentry.hpp"

namespace MWWorld
{
    class World;
}

namespace MWDialogue
{
    /// \brief Collection of seen responses for a topic
    class Topic
    {
        public:

            typedef std::vector<std::string> TEntryContainer;
            typedef TEntryContainer::const_iterator TEntryIter;

        protected:

            std::string mTopic;
            TEntryContainer mEntries; // info-IDs

        public:

            Topic();

            Topic (const std::string& topic);

            virtual ~Topic();

            virtual void addEntry (const JournalEntry& entry, const MWWorld::World& world);
            ///< Add entry
            ///
            /// \note Redundant entries are ignored.

            TEntryIter begin();
            ///< Iterator pointing to the begin of the journal for this topic.

            TEntryIter end();
            ///< Iterator pointing past the end of the journal for this topic.

            JournalEntry getEntry (const std::string& infoId);
    };
}

#endif
