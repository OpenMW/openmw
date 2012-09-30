#ifndef GAME_MWWORLD_ACTIONTELEPORT_H
#define GAME_MWWORLD_ACTIONTELEPORT_H

#include <string>

#include <components/esm/defs.hpp>

#include "action.hpp"

namespace MWWorld
{
    class ActionTeleport : public Action
    {
            std::string mCellName;
            ESM::Position mPosition;

            virtual void executeImp (const Ptr& actor);

        public:

            ActionTeleport (const std::string& cellName, const ESM::Position& position);
            ///< If cellName is empty, an exterior cell is asumed.
    };
}

#endif
