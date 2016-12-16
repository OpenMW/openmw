#include "../mwbase/environment.hpp"
#include "../mwworld/worldimp.hpp"
#include <components/esm/cellid.hpp>
#include <components/openmw-mp/Log.hpp>
#include "../mwworld/containerstore.hpp"
#include "../mwworld/class.hpp"

#include "WorldController.hpp"
#include "Main.hpp"
#include "LocalPlayer.hpp"


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


void mwmp::WorldController::openContainer(const MWWorld::Ptr &container, bool loot)
{
    LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Container \"%s\" (%d) is openned. Loot: %s",
                       container.getCellRef().getRefId().c_str(),
                       container.getCellRef().getRefNum().mIndex,
                       loot ? "true" : "false");

    MWWorld::ContainerStore &cont = container.getClass().getContainerStore(container);
    for (MWWorld::ContainerStoreIterator iter = cont.begin(); iter != cont.end(); iter++)
    {
        int count = iter->getRefData().getCount();
        const std::string &name = iter->getCellRef().getRefId();

        LOG_APPEND(Log::LOG_VERBOSE, " - Item. Refid: \"%s\" Count: %d", name.c_str(), count);

        /*if(::Misc::StringUtils::ciEqual(name, "gold_001"))
            cont.remove("gold_001", count, container);*/
    }

}

void mwmp::WorldController::closeContainer(const MWWorld::Ptr &container)
{
    LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Container \"%s\" (%d) is closed.",
                       container.getCellRef().getRefId().c_str(),
                       container.getCellRef().getRefNum().mIndex);

    MWWorld::ContainerStore &cont = container.getClass().getContainerStore(container);
    for (MWWorld::ContainerStoreIterator iter = cont.begin(); iter != cont.end(); iter++)
    {
        LOG_APPEND(Log::LOG_VERBOSE, " - Item. Refid: \"%s\" Count: %d",
                   iter->getCellRef().getRefId().c_str(), iter->getRefData().getCount());
    }
    mwmp::Main::get().getLocalPlayer()->sendInventory(); // FIXME: must send only if there is a changes in the inventory.
}
