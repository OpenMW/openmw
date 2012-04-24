#ifndef GAME_MWWORLD_ACTIONTALK_H
#define GAME_MWWORLD_ACTIONTALK_H

#include "ptr.hpp"
#include "action.hpp"

namespace MWWorld
{
    class ActionTalk : public Action
    {
            Ptr mActor;

        public:

            ActionTalk (const Ptr& actor);
            ///< \param actor The actor the player is talking to

            virtual void execute();
    };
}

#endif
