#ifndef GAME_MWWORLD_ACTIONALCHEMY_H
#define GAME_MWWORLD_ACTIONALCHEMY_H

#include "action.hpp"

namespace MWWorld
{
    class ActionAlchemy : public Action
    {
        bool mForce;
        virtual void executeImp (const Ptr& actor);

    public:
        ActionAlchemy(bool force=false);
    };
}

#endif
