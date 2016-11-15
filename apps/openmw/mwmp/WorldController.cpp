#include <apps/openmw/mwbase/environment.hpp>
#include <apps/openmw/mwworld/worldimp.hpp>
#include <components/esm/cellid.hpp>
#include <components/openmw-mp/Log.hpp>


#include "WorldController.hpp"
#include "Main.hpp"


mwmp::WorldController::WorldController()
{

}

mwmp::WorldController::~WorldController()
{

}

MWWorld::CellStore *mwmp::WorldController::getCell(const ESM::Cell& cell)
{
    MWWorld::CellStore *cellStore;

    if (cell.isExterior())
        cellStore = MWBase::Environment::get().getWorld()->getExterior(cell.mData.mX, cell.mData.mY);
    else
    {
        try
        {
            cellStore = MWBase::Environment::get().getWorld()->getInterior(cell.mName);
        }
        catch (std::exception&)
        {
            cellStore = NULL;
        }
    }

    return cellStore;
}
