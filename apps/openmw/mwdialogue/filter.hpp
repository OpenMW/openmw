#ifndef GAME_MWDIALOGUE_FILTER_H
#define GAME_MWDIALOGUE_FILTER_H

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

            bool operator() (const ESM::DialInfo& info) const;    
            ///< \return does the dialogue match?
            
            const ESM::DialInfo *search (const ESM::Dialogue& dialogue) const;
    };
}

#endif
