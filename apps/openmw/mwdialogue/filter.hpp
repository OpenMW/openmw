#ifndef GAME_MWDIALOGUE_FILTER_H
#define GAME_MWDIALOGUE_FILTER_H

#include <vector>

#include "../mwworld/ptr.hpp"

namespace ESM
{
    struct DialInfo;
    struct Dialogue;
}

namespace MWDialogue
{
    class SelectWrapper;

    class Filter
    {
            MWWorld::Ptr mActor;
            int mChoice;
            bool mTalkedToPlayer;

            bool testActor (const ESM::DialInfo& info) const;
            ///< Is this the right actor for this \a info?

            bool testPlayer (const ESM::DialInfo& info) const;
            ///< Do the player and the cell the player is currently in match \a info?

            bool testSelectStructs (const ESM::DialInfo& info) const;
            ///< Are all select structs matching?

            bool testDisposition (const ESM::DialInfo& info, bool invert=false) const;
            ///< Is the actor disposition toward the player high enough (or low enough, if \a invert is true)?

            bool testSelectStruct (const SelectWrapper& select) const;

            bool testSelectStructNumeric (const SelectWrapper& select) const;

            int getSelectStructInteger (const SelectWrapper& select) const;

            bool getSelectStructBoolean (const SelectWrapper& select) const;

            int getFactionRank (const MWWorld::Ptr& actor, const std::string& factionId) const;

            bool hasFactionRankSkillRequirements (const MWWorld::Ptr& actor, const std::string& factionId,
                int rank) const;

            bool hasFactionRankReputationRequirements (const MWWorld::Ptr& actor, const std::string& factionId,
                int rank) const;

        public:

            Filter (const MWWorld::Ptr& actor, int choice, bool talkedToPlayer);

            std::vector<const ESM::DialInfo *> list (const ESM::Dialogue& dialogue,
                bool fallbackToInfoRefusal, bool searchAll, bool invertDisposition=false) const;

            const ESM::DialInfo* search (const ESM::Dialogue& dialogue, const bool fallbackToInfoRefusal) const;
            ///< Get a matching response for the requested dialogue.
            ///  Redirect to "Info Refusal" topic if a response fulfills all conditions but disposition.

            bool responseAvailable (const ESM::Dialogue& dialogue) const;
            ///< Does a matching response exist? (disposition is ignored for this check)
    };
}

#endif
