#ifndef GAME_MWWORLD_ACTIONTELEPORT_H
#define GAME_MWWORLD_ACTIONTELEPORT_H

#include <set>
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

            /// If cellName is empty, an exterior cell is assumed.
            /// @param teleportFollowers Whether to teleport any following actors of the target actor as well.
            ActionTeleport (const std::string& cellName, const ESM::Position& position, bool teleportFollowers);

            /// Outputs every actor follower who is in teleport range and wasn't ordered to not enter interiors
            static void getFollowersToTeleport(const MWWorld::Ptr& actor, std::set<MWWorld::Ptr>& out);
    };
}

#endif
