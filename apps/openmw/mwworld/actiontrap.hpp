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

            virtual void executeImp (const Ptr& actor);

        public:

            /// @param spellId
            /// @param actor Actor that activated the trap
            /// @param trapSource
            ActionTrap (const Ptr& actor, const std::string& spellId, const Ptr& trapSource)
                : Action(false, actor), mSpellId(spellId), mTrapSource(trapSource) {}
    };
}


#endif
