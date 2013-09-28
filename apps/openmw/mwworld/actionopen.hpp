
#ifndef GAME_MWWORLD_ACTIONOPEN_H
#define GAME_MWWORLD_ACTIONOPEN_H

#include "action.hpp"
#include "ptr.hpp"


namespace MWWorld
{
    class ActionOpen : public Action
    {
            Ptr mContainer;

            virtual void executeImp (const MWWorld::Ptr& actor);

        public:
            ActionOpen (const Ptr& container);
            ///< \param The Container the Player has activated.
    };
}

#endif // ACTIONOPEN_H
