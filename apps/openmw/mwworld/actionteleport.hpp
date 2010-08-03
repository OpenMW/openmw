#ifndef GAME_MWWORLD_ACTIONTELEPORT_H
#define GAME_MWWORLD_ACTIONTELEPORT_H

#include <string>

#include <components/esm/defs.hpp>

#include "action.hpp"

namespace MWWorld
{
    class ActionTeleportPlayer : public Action
    {
            std::string mCellName;
            ESM::Position mPosition;

        public:

            ActionTeleportPlayer (const std::string& cellName, const ESM::Position& position);

            virtual void execute (Environment& environment);
    };
}

#endif
