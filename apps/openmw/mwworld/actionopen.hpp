#ifndef GAME_MWWORLD_ACTIONOPEN_H
#define GAME_MWWORLD_ACTIONOPEN_H

#include "action.hpp"

namespace MWWorld
{
    class ActionOpen : public Action
    {
            void executeImp (const MWWorld::Ptr& actor) override;

        public:
            ActionOpen (const Ptr& container);
            ///< \param container The Container the Player has activated.

    };
}

#endif // ACTIONOPEN_H
