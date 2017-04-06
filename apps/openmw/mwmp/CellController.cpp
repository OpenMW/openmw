#include "../mwbase/environment.hpp"
#include "../mwworld/worldimp.hpp"
#include <components/esm/cellid.hpp>
#include <components/openmw-mp/Log.hpp>
#include "../mwworld/containerstore.hpp"
#include "../mwworld/class.hpp"

#include "CellController.hpp"
#include "Main.hpp"
#include "LocalPlayer.hpp"
using namespace mwmp;

std::deque<mwmp::Cell *> CellController::cellsActive;

mwmp::CellController::CellController()
{

}

mwmp::CellController::~CellController()
{

}

void CellController::updateLocal()
{
    for (std::deque<mwmp::Cell *>::iterator it = cellsActive.begin(); it != cellsActive.end();)
    {
        mwmp::Cell *mpCell = *it;

        if (!MWBase::Environment::get().getWorld()->isCellActive(mpCell->getCellStore()))
        {
            mpCell->uninitializeLocalActors();
            it = cellsActive.erase(it);
        }
        else
        {
            //LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Updating mwmp::Cell %s", mpCell->getDescription().c_str());
            mpCell->updateLocal();
            ++it;
        }
    }
}

void CellController::initializeCellLocal(const ESM::Cell& cell)
{
    MWWorld::CellStore *cellStore = getCell(cell);

    if (!cellStore) return;

    mwmp::Cell *mpCell = new mwmp::Cell(cellStore);
    
    LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Initialized mwmp::Cell %s", mpCell->getDescription().c_str());

    mpCell->initializeLocalActors();
    cellsActive.push_back(mpCell);
}

void CellController::readCellFrame(mwmp::WorldEvent& worldEvent)
{
    bool cellExisted = false;

    // Check if this cell already exists
    for (std::deque<mwmp::Cell *>::iterator it = cellsActive.begin(); it != cellsActive.end(); ++it)
    {
        mwmp::Cell *mpCell = *it;

        if (worldEvent.cell.getDescription() == mpCell->getDescription())
        {
            mpCell->readCellFrame(worldEvent);
            cellExisted = true;
            break;
        }
    }

    if (!cellExisted)
    {
        MWWorld::CellStore *cellStore = getCell(worldEvent.cell);

        if (!cellStore) return;

        mwmp::Cell *mpCell = new mwmp::Cell(cellStore);

        LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Initialized mwmp::Cell %s", mpCell->getDescription().c_str());

        cellsActive.push_back(mpCell);
        mpCell->readCellFrame(worldEvent);
    }
}

int mwmp::CellController::getCellSize() const
{
    return 8192;
}

MWWorld::CellStore *mwmp::CellController::getCell(const ESM::Cell& cell)
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


void mwmp::CellController::openContainer(const MWWorld::Ptr &container, bool loot)
{
    // Record this as the player's current open container
    mwmp::Main::get().getLocalPlayer()->storeCurrentContainer(container, loot);

    LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Container \"%s\" (%d) is opened. Loot: %s",
                       container.getCellRef().getRefId().c_str(), container.getCellRef().getRefNum().mIndex,
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

void mwmp::CellController::closeContainer(const MWWorld::Ptr &container)
{
    mwmp::Main::get().getLocalPlayer()->clearCurrentContainer();

    // If the player died while in a container, the container's Ptr could be invalid now
    if (!container.isEmpty())
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Container \"%s\" (%d) is closed.", container.getCellRef().getRefId().c_str(),
                           container.getCellRef().getRefNum().mIndex);

        MWWorld::ContainerStore &cont = container.getClass().getContainerStore(container);
        for (MWWorld::ContainerStoreIterator iter = cont.begin(); iter != cont.end(); iter++)
        {
            LOG_APPEND(Log::LOG_VERBOSE, " - Item. Refid: \"%s\" Count: %d", iter->getCellRef().getRefId().c_str(),
                       iter->getRefData().getCount());
        }
    }

    mwmp::Main::get().getLocalPlayer()->updateInventory();
}
