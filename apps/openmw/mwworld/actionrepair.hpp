#ifndef GAME_MWWORLD_ACTIONREPAIR_H
#define GAME_MWWORLD_ACTIONREPAIR_H

#include "action.hpp"

namespace MWWorld
{
    class ActionRepair : public Action
    {
        bool mForce;

        virtual void executeImp (const Ptr& actor);

    public:
        /// @param item repair hammer
        ActionRepair(const Ptr& item, bool force=false);
    };
}

#endif
