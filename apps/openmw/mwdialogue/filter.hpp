#ifndef GAME_MWDIALOGUE_FILTER_H
#define GAME_MWDIALOGUE_FILTER_H

#include "../mwworld/ptr.hpp"

namespace ESM
{
    struct DialInfo;
}

namespace MWDialogue
{
    class Filter
    {
            MWWorld::Ptr mActor;
    
            bool testActor (const ESM::DialInfo& info) const;
            ///< Is this the right actor for this \a info?
    
            bool testPlayer (const ESM::DialInfo& info) const;
            ///< Do the player and the cell the player is currently in match \a info?
    
        public:
        
            Filter (const MWWorld::Ptr& actor);    

            bool operator() (const ESM::DialInfo& info) const;    
            ///< \return does the dialogue match?
    };
}

#endif
