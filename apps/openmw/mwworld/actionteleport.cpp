
#include "actionteleport.hpp"

#include "environment.hpp"
#include "world.hpp"

namespace MWWorld
{
    ActionTeleportPlayer::ActionTeleportPlayer (const std::string& cellName,
        const ESM::Position& position)
    : mCellName (cellName), mPosition (position)
    {}

    void ActionTeleportPlayer::ActionTeleportPlayer::execute (Environment& environment)
    {
        environment.mWorld->changeCell (mCellName, mPosition);
    }
}
