
#include "actionteleport.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

namespace MWWorld
{
    ActionTeleport::ActionTeleport (const std::string& cellName,
        const ESM::Position& position)
    : mCellName (cellName), mPosition (position)
    {
    	teleport = true;
    }

    void ActionTeleport::executeImp (const Ptr& actor)
    {
    	teleport = true;

        if (mCellName.empty())
            MWBase::Environment::get().getWorld()->changeToExteriorCell (mPosition);
        else
            MWBase::Environment::get().getWorld()->changeToInteriorCell (mCellName, mPosition);
    }
}
