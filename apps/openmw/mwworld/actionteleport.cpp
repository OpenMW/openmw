
#include "actionteleport.hpp"

#include "environment.hpp"
#include "world.hpp"

namespace MWWorld
{
    ActionTeleportPlayer::ActionTeleportPlayer (const std::string& cellName,
        const ESM::Position& position)
    : mCellName (cellName), mPosition (position)
    {}

    void ActionTeleportPlayer::execute (Environment& environment)
    {
        if (mCellName.empty())
            environment.mWorld->changeToExteriorCell (mPosition);
        else
            environment.mWorld->changeCell (mCellName, mPosition);
    }
}
