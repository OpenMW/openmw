#ifndef GAME_MWWORLD_ACTIONDOOR_H
#define GAME_MWWORLD_ACTIONDOOR_H

#include "action.hpp"
#include "ptr.hpp"

namespace MWWorld
{
    class ActionDoor : public Action
    {
            virtual void executeImp (const MWWorld::Ptr& actor);

        public:
            ActionDoor (const Ptr& object);
    };
}

#endif
