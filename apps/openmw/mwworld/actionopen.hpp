
#ifndef GAME_MWWORLD_ACTIONOPEN_H
#define GAME_MWWORLD_ACTIONOPEN_H

#include "action.hpp"
#include "ptr.hpp"


namespace MWWorld
{
    class ActionOpen : public Action
    {
            virtual void executeImp (const MWWorld::Ptr& actor);

        public:
            ActionOpen (const Ptr& container, bool loot=false);
            ///< \param container The Container the Player has activated.
            /// \param loot If true, display the "dispose of corpse" button

        private:
            bool mLoot;
    };
}

#endif // ACTIONOPEN_H
