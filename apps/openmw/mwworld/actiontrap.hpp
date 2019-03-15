#ifndef GAME_MWWORLD_ACTIONTRAP_H
#define GAME_MWWORLD_ACTIONTRAP_H

#include <string>

#include "action.hpp"

namespace MWWorld
{
    class ActionTrap : public Action
    {
            std::string mSpellId;
            MWWorld::Ptr mTrapSource;

            virtual void executeImp (const Ptr& actor);

        public:

            /// @param spellId
            /// @param trapSource
            ActionTrap (const std::string& spellId, const Ptr& trapSource)
                : Action(false, trapSource), mSpellId(spellId), mTrapSource(trapSource) {}
    };
}


#endif
