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
            bool mTeleportFollowers;

            /// Teleports this actor and also teleports anyone following that actor.
            virtual void executeImp (const Ptr& actor);

            /// Teleports only the given actor (internal use).
            void teleport(const Ptr &actor);

        public:

            ActionTeleport (const std::string& cellName, const ESM::Position& position, bool teleportFollowers);
            ///< If cellName is empty, an exterior cell is assumed.
            /// @param teleportFollowers Whether to teleport any following actors of the target actor as well.
    };
}

#endif
