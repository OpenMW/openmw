#ifndef GAME_MWWORLD_ACTIONSOULGEM_H
#define GAME_MWWORLD_ACTIONSOULGEM_H

#include "action.hpp"

namespace MWWorld
{
    class ActionSoulgem : public Action
    {
            void executeImp (const MWWorld::Ptr& actor) override;

        public:
            /// @param soulgem to use
            ActionSoulgem (const Ptr& object);
    };
}

#endif
