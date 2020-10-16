#ifndef GAME_MWWORLD_ACTIONTAKE_H
#define GAME_MWWORLD_ACTIONTAKE_H

#include "action.hpp"

namespace MWWorld
{
    class ActionTake : public Action
    {
            void executeImp (const Ptr& actor) override;

        public:

            ActionTake (const MWWorld::Ptr& object);
    };
}

#endif
