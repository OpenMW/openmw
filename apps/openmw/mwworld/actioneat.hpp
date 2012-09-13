#ifndef GAME_MWWORLD_ACTIONEAT_H
#define GAME_MWWORLD_ACTIONEAT_H

#include "action.hpp"
#include "ptr.hpp"

namespace MWWorld
{
    class ActionEat : public Action
    {
            virtual void executeImp (const Ptr& actor);

        public:

            ActionEat (const MWWorld::Ptr& object);
    };
}

#endif
