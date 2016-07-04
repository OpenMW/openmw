#ifndef GAME_MWWORLD_ACTIONTRAP_H
#define GAME_MWWORLD_ACTIONTRAP_H

#include <string>

#include "action.hpp"
#include "ptr.hpp"

namespace MWWorld
{
    class ActionTrap : public Action
    {
            std::string mSpellId;
            MWWorld::Ptr mTrapSource;

            /// Activating trapped object without telekinesis active or within trap range
            virtual void executeImp (const Ptr& actor);

            /// Activating trapped object with telekinesis active
            virtual void executeImp (const Ptr& actor, float distanceToObject);

        public:

            /// @param spellId
            /// @param actor Actor that activated the trap
            /// @param trapSource
            ActionTrap (const Ptr& actor, const std::string& spellId, const Ptr& trapSource)
                : Action(false, actor), mSpellId(spellId), mTrapSource(trapSource) {}
    };
}


#endif
