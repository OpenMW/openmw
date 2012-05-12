#ifndef GAME_MWWORLD_ACTIONREAD_H
#define GAME_MWWORLD_ACTIONREAD_H

#include "action.hpp"
#include "ptr.hpp"

namespace MWWorld
{
    class ActionRead : public Action
    {
            Ptr mObject; // book or scroll to read

        public:
            /// @param book or scroll to read
            ActionRead (const Ptr& object);

            virtual void execute ();
    };
}

#endif // ACTIONOPEN_H
