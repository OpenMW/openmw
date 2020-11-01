#ifndef GAME_MWWORLD_NULLACTION_H
#define GAME_MWWORLD_NULLACTION_H

#include "action.hpp"

namespace MWWorld
{
    /// \brief Action: do nothing
    class NullAction : public Action
    {
            void executeImp (const Ptr& actor) override {}

            bool isNullAction() override { return true; }
    };
}

#endif
