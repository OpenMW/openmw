#ifndef GAME_MWWORLD_ACTIONTELEPORT_H
#define GAME_MWWORLD_ACTIONTELEPORT_H

#include <set>
#include <string>
#include <string_view>

#include <components/esm/defs.hpp>

#include "action.hpp"

namespace MWWorld
{
    class ActionTeleport : public Action
    {
        ESM::RefId mCellId;
        ESM::Position mPosition;
        bool mTeleportFollowers;

        /// Teleports this actor and also teleports anyone following that actor.
        void executeImp(const Ptr& actor) override;

        /// Teleports only the given actor (internal use).
        void teleport(const Ptr& actor);

    public:
        /// If cellName is empty, an exterior cell is assumed.
        /// @param teleportFollowers Whether to teleport any following actors of the target actor as well.
        ActionTeleport(ESM::RefId cellId, const ESM::Position& position, bool teleportFollowers);

        /// @param includeHostiles If true, include hostile followers (which won't actually be teleported) in the
        /// output,
        ///                        e.g. so that the teleport action can calm them.
        static void getFollowers(
            const MWWorld::Ptr& actor, std::set<MWWorld::Ptr>& out, bool toExterior, bool includeHostiles = false);
    };
}

#endif
