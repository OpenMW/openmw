#ifndef GAME_MWWORLD_ACTIONEAT_H
#define GAME_MWWORLD_ACTIONEAT_H

#include "action.hpp"

namespace MWWorld
{
    class ActionEat : public Action
    {
            void executeImp (const Ptr& actor) override;

        public:

            ActionEat (const MWWorld::Ptr& object);
    };
}

#endif
