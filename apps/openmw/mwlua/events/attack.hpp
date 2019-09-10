#ifndef GAME_MWLUA_EVENT_ATTACK_H
#define GAME_MWLUA_EVENT_ATTACK_H

#include "../objectfilteredevent.hpp"
#include "../disableableevent.hpp"

#include "../../mwworld/ptr.hpp"

namespace MWLua
{
    namespace Event
    {
        class AttackEvent : public ObjectFilteredEvent, public DisableableEvent<AttackEvent>
        {
        public:
            AttackEvent(const MWWorld::Ptr& attacker, const MWWorld::Ptr& target);
            sol::table createEventTable();

        protected:
            MWWorld::Ptr mAttacker;
            MWWorld::Ptr mTarget;
        };
    }
}

#endif
