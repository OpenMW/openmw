#ifndef GAME_MWWORLD_ACTIONALCHEMY_H
#define GAME_MWWORLD_ACTIONALCHEMY_H

#include "action.hpp"

namespace MWWorld
{
    class ActionAlchemy : public Action
    {
            virtual void executeImp (const Ptr& actor);
    };
}

#endif
