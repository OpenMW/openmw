#ifndef GAME_MMDIALOGUE_JOURNALENTRY_H
#define GAME_MWDIALOGUE_JOURNALENTRY_H

#include <string>

namespace ESMS
{
    struct ESMStore;
}

namespace MWWorld
{
    class World;
}

namespace MWDialogue
{
    /// \brief a quest or dialogue entry with a timestamp
    struct JournalEntry
    {
        int mDay;
        std::string mTopic;
        std::string mInfoId;

        JournalEntry();

        JournalEntry (int day, const std::string& topic, const std::string& infoId);

        std::string getText (const ESMS::ESMStore& store) const;

        static JournalEntry makeFromQuest (const std::string& topic, int index,
            const MWWorld::World& world);
    };

}

#endif
