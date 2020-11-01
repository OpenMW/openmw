#ifndef GAME_MWWORLD_ACTIONTALK_H
#define GAME_MWWORLD_ACTIONTALK_H

#include "action.hpp"

namespace MWWorld
{
    class ActionTalk : public Action
    {
            void executeImp (const Ptr& actor) override;

        public:

            ActionTalk (const Ptr& actor);
            ///< \param actor The actor the player is talking to
    };
}

#endif
