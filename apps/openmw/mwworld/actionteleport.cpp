
#include "actionteleport.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "player.hpp"

namespace MWWorld
{
    ActionTeleport::ActionTeleport (const std::string& cellName,
        const ESM::Position& position)
    : Action (true), mCellName (cellName), mPosition (position)
    {
    }

    void ActionTeleport::executeImp (const Ptr& actor)
    {
        MWBase::World* world = MWBase::Environment::get().getWorld();
        world->getPlayer().setTeleported(true);

        if (mCellName.empty())
            world->changeToExteriorCell (mPosition);
        else
            world->changeToInteriorCell (mCellName, mPosition);
    }
}
