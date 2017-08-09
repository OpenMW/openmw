#ifndef GAME_MWWORLD_ACTIONREPAIR_H
#define GAME_MWWORLD_ACTIONREPAIR_H

#include "action.hpp"

namespace MWWorld
{
    class ActionRepair : public Action
    {
            virtual void executeImp (const Ptr& actor);

    public:
        ActionRepair(const MWWorld::Ptr& item);
    };
}

#endif
