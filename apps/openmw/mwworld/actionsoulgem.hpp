#ifndef GAME_MWWORLD_ACTIONSOULGEM_H
#define GAME_MWWORLD_ACTIONSOULGEM_H

#include "action.hpp"
#include "ptr.hpp"

namespace MWWorld
{
    class ActionSoulgem : public Action
    {
            virtual void executeImp (const MWWorld::Ptr& actor);

        public:
            /// @param soulgem to use
            ActionSoulgem (const Ptr& object);
    };
}

#endif
