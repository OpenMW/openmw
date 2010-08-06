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

            virtual void execute (Environment& environment);
    };
}

#endif
