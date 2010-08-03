#ifndef GAME_MWWORLD_NULLACTION_H
#define GAME_MWWORLD_NULLACTION_H

#include "action.hpp"

namespace MWWorld
{
    /// \brief Action: do nothing
    class NullAction : public Action
    {
        public:

            virtual void execute (Environment& environment) {}
    };
}

#endif
