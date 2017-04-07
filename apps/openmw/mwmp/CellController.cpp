#include "../mwbase/environment.hpp"
#include "../mwworld/worldimp.hpp"
#include <components/esm/cellid.hpp>
#include <components/openmw-mp/Log.hpp>
#include <components/openmw-mp/Utils.hpp>
#include "../mwworld/containerstore.hpp"
#include "../mwworld/class.hpp"

#include "CellController.hpp"
#include "Main.hpp"
#include "LocalActor.hpp"
#include "LocalPlayer.hpp"
using namespace mwmp;

std::map<std::string, mwmp::Cell *> CellController::cellsActive;
std::map<std::string, std::string> CellController::localActorsToCells;

mwmp::CellController::CellController()
{

}

mwmp::CellController::~CellController()
{

}

void CellController::updateLocal()
{
    for (std::map<std::string, mwmp::Cell *>::iterator it = cellsActive.begin(); it != cellsActive.end();)
    {
        mwmp::Cell *mpCell = it->second;

        if (!MWBase::Environment::get().getWorld()->isCellActive(mpCell->getCellStore()))
        {
            mpCell->uninitializeLocalActors();
            cellsActive.erase(it++);
        }
        else
        {
            //LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Updating mwmp::Cell %s", mpCell->getDescription().c_str());
            mpCell->updateLocal();
            ++it;
        }
    }
}

void CellController::updateDedicated(float dt)
{
    for (std::map<std::string, mwmp::Cell *>::iterator it = cellsActive.begin(); it != cellsActive.end(); ++it)
    {
        it->second->updateDedicated(dt);
    }
}

void CellController::initializeCellLocal(const ESM::Cell& cell)
{
    MWWorld::CellStore *cellStore = getCell(cell);

    if (!cellStore) return;

    mwmp::Cell *mpCell = new mwmp::Cell(cellStore);

    LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Initialized mwmp::Cell %s", mpCell->getDescription().c_str());

    mpCell->initializeLocalActors();

    std::string mapIndex = mpCell->getDescription();
    cellsActive[mapIndex] = mpCell;
}

void CellController::readCellFrame(WorldEvent& worldEvent)
{
    std::string mapIndex = worldEvent.cell.getDescription();

    // If this key doesn't exist, create it
    if (cellsActive.count(mapIndex) == 0)
    {
        MWWorld::CellStore *cellStore = getCell(worldEvent.cell);

        if (!cellStore) return;

        mwmp::Cell *mpCell = new mwmp::Cell(cellStore);
        cellsActive[mapIndex] = mpCell;
    }

    // If this now exists, send it the data
    if (cellsActive.count(mapIndex) > 0)
    {
        cellsActive[mapIndex]->readCellFrame(worldEvent);
    }
}

void CellController::setLocalActorRecord(std::string actorIndex, std::string cellIndex)
{
    localActorsToCells[actorIndex] = cellIndex;
}

void CellController::removeLocalActorRecord(std::string actorIndex)
{
    localActorsToCells.erase(actorIndex);
}

bool CellController::hasLocalActorRecord(MWWorld::Ptr ptr)
{
    std::string mapIndex = generateMapIndex(ptr);

    return (localActorsToCells.count(mapIndex) > 0);
}

LocalActor *CellController::getLocalActor(MWWorld::Ptr ptr)
{
    std::string actorIndex = generateMapIndex(ptr);
    std::string cellIndex = localActorsToCells.at(actorIndex);

    return cellsActive.at(cellIndex)->getLocalActor(actorIndex);
}

std::string CellController::generateMapIndex(MWWorld::Ptr ptr)
{
    std::string mapIndex = "";
    mapIndex += ptr.getCellRef().getRefId();
    mapIndex += "-" + Utils::toString(ptr.getCellRef().getRefNum().mIndex);
    mapIndex += "-" + Utils::toString(ptr.getCellRef().getMpNum());
    return mapIndex;
}

std::string CellController::generateMapIndex(WorldObject object)
{
    std::string mapIndex = "";
    mapIndex += object.refId;
    mapIndex += "-" + Utils::toString(object.refNumIndex);
    mapIndex += "-" + Utils::toString(object.mpNum);
    return mapIndex;
}

int CellController::getCellSize() const
{
    return 8192;
}

MWWorld::CellStore *CellController::getCell(const ESM::Cell& cell)
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


void CellController::openContainer(const MWWorld::Ptr &container, bool loot)
{
    // Record this as the player's current open container
    Main::get().getLocalPlayer()->storeCurrentContainer(container, loot);

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

void CellController::closeContainer(const MWWorld::Ptr &container)
{
    Main::get().getLocalPlayer()->clearCurrentContainer();

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

    Main::get().getLocalPlayer()->updateInventory();
}
