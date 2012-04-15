
#ifndef GAME_MWWORLD_ACTIONOPEN_H
#define GAME_MWWORLD_ACTIONOPEN_H

#include "action.hpp"
#include "ptr.hpp"


namespace MWWorld
{
    class ActionOpen : public Action
    {
            Ptr mContainer;

        public:
            ActionOpen (const Ptr& container);
            ///< \param The Container the Player has activated.
            virtual void execute (Environment& environment);
    };
}

#endif // ACTIONOPEN_H
