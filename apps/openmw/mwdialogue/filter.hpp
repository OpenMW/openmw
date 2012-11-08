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
    
        public:
        
            Filter (const MWWorld::Ptr& actor);    

            bool operator() (const ESM::DialInfo& dialogue);    
            ///< \return does the dialogue match?
    };
}

#endif
