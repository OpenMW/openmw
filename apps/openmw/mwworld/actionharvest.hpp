#ifndef GAME_MWWORLD_ACTIONHARVEST_H
#define GAME_MWWORLD_ACTIONHARVEST_H

#include "action.hpp"
#include "ptr.hpp"

namespace MWWorld
{
    class ActionHarvest : public Action
    {
            void executeImp (const MWWorld::Ptr& actor) override;

        public:
            ActionHarvest (const Ptr& container);
            ///< \param container The Container the Player has activated.
    };
}

#endif // ACTIONOPEN_H
