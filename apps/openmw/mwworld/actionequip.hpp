#ifndef GAME_MWWORLD_ACTIONEQUIP_H
#define GAME_MWWORLD_ACTIONEQUIP_H

#include "action.hpp"
#include "ptr.hpp"

namespace MWWorld
{
    class ActionEquip : public Action
    {
            virtual void executeImp (const Ptr& actor);

        public:
            /// @param item to equip
            ActionEquip (const Ptr& object);
    };
}

#endif
