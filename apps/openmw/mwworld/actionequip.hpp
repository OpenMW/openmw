#ifndef GAME_MWWORLD_ACTIONEQUIP_H
#define GAME_MWWORLD_ACTIONEQUIP_H

#include "action.hpp"
#include "ptr.hpp"

namespace MWWorld
{
    class ActionEquip : public Action
    {
            Ptr mObject;

        public:
            /// @param item to equip
            ActionEquip (const Ptr& object);

            virtual void execute ();
    };
}

#endif
