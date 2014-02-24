#include "referenceinterface.hpp"

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

namespace MWGui
{
    ReferenceInterface::ReferenceInterface()
        : mCurrentPlayerCell(NULL)
    {
    }

    ReferenceInterface::~ReferenceInterface()
    {
    }

    void ReferenceInterface::checkReferenceAvailable()
    {
        MWWorld::CellStore* playerCell = MWBase::Environment::get().getWorld()->getPlayerPtr().getCell();

        // check if player has changed cell, or count of the reference has become 0
        if ((playerCell != mCurrentPlayerCell && mCurrentPlayerCell != NULL)
             || (!mPtr.isEmpty() && mPtr.getRefData().getCount() == 0))
        {
            if (!mPtr.isEmpty())
            {
                mPtr = MWWorld::Ptr();
                onReferenceUnavailable();
            }
        }

        mCurrentPlayerCell = playerCell;
    }
}
