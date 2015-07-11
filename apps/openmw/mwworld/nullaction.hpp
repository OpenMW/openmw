#ifndef GAME_MWWORLD_nullptrACTION_H
#define GAME_MWWORLD_nullptrACTION_H

#include "action.hpp"

namespace MWWorld
{
    /// \brief Action: do nothing
    class NullAction : public Action
    {
            virtual void executeImp (const Ptr& actor) {}
    };
}

#endif
