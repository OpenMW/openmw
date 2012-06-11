#include "referenceinterface.hpp"

#include "../mwworld/player.hpp"
#include "../mwworld/world.hpp"
#include "../mwbase/environment.hpp"

namespace MWGui
{
    ReferenceInterface::ReferenceInterface()
        : mCurrentPlayerCell(NULL)
    {
    }

    void ReferenceInterface::checkReferenceAvailable()
    {
        if (mPtr.isEmpty())
            return;

        MWWorld::Ptr::CellStore* playerCell = MWBase::Environment::get().getWorld()->getPlayer().getPlayer().getCell();

        // check if player has changed cell, or count of the reference has become 0
        if ((playerCell != mCurrentPlayerCell && mCurrentPlayerCell != NULL)
            || mPtr.getRefData().getCount() == 0)
            onReferenceUnavailable();

        mCurrentPlayerCell = playerCell;
    }
}
