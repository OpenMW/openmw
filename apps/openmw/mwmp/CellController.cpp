#include <components/esm/cellid.hpp>
#include <components/openmw-mp/Log.hpp>
#include <components/openmw-mp/Utils.hpp>

#include "../mwbase/environment.hpp"

#include "../mwworld/containerstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/worldimp.hpp"

#include "CellController.hpp"
#include "Main.hpp"
#include "LocalActor.hpp"
#include "LocalPlayer.hpp"
using namespace mwmp;

std::map<std::string, mwmp::Cell *> CellController::cellsInitialized;
std::map<std::string, std::string> CellController::localActorsToCells;
std::map<std::string, std::string> CellController::dedicatedActorsToCells;

mwmp::CellController::CellController()
{

}
void CellController::updateLocal(bool forceUpdate)
{
    // Loop through Cells, deleting inactive ones and updating LocalActors in active ones
    for (std::map<std::string, mwmp::Cell *>::iterator it = cellsInitialized.begin(); it != cellsInitialized.end();)
    {
        mwmp::Cell *mpCell = it->second;

        if (!MWBase::Environment::get().getWorld()->isCellActive(mpCell->getCellStore()))
        {
            mpCell->uninitializeLocalActors();
            mpCell->uninitializeDedicatedActors();
            delete it->second;
            cellsInitialized.erase(it++);
        }
        else
        {
            mpCell->updateLocal(forceUpdate);
            ++it;
        }
    }

    // Loop through Cells and initialize new LocalActors for eligible ones
    //
    // Note: This cannot be combined with the above loop because initializing LocalActors in a Cell before they are
    //       deleted from their previous one can make their records stay deleted
    for (std::map<std::string, mwmp::Cell *>::iterator it = cellsInitialized.begin(); it != cellsInitialized.end(); ++it)
    {
        mwmp::Cell *mpCell = it->second;
        if (mpCell->shouldInitializeActors == true)
        {
            mpCell->shouldInitializeActors = false;
            mpCell->initializeLocalActors();
        }
    }
}

void CellController::updateDedicated(float dt)
{
    for (std::map<std::string, mwmp::Cell *>::iterator it = cellsInitialized.begin(); it != cellsInitialized.end(); ++it)
    {
        it->second->updateDedicated(dt);
    }
}

void CellController::initializeCell(const ESM::Cell& cell)
{
    std::string mapIndex = cell.getDescription();

    // If this key doesn't exist, create it
    if (cellsInitialized.count(mapIndex) == 0)
    {
        MWWorld::CellStore *cellStore = getCellStore(cell);

        if (!cellStore) return;

        mwmp::Cell *mpCell = new mwmp::Cell(cellStore);
        cellsInitialized[mapIndex] = mpCell;

        LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "- Initialized mwmp::Cell %s", mpCell->getDescription().c_str());
    }
}

void CellController::readPositions(ActorList& actorList)
{
    std::string mapIndex = actorList.cell.getDescription();

    initializeCell(actorList.cell);

    // If this now exists, send it the data
    if (cellsInitialized.count(mapIndex) > 0)
    {
        cellsInitialized[mapIndex]->readPositions(actorList);
    }
}

void CellController::readAnimFlags(ActorList& actorList)
{
    std::string mapIndex = actorList.cell.getDescription();

    initializeCell(actorList.cell);

    // If this now exists, send it the data
    if (cellsInitialized.count(mapIndex) > 0)
    {
        cellsInitialized[mapIndex]->readAnimFlags(actorList);
    }
}

void CellController::readAnimPlay(ActorList& actorList)
{
    std::string mapIndex = actorList.cell.getDescription();

    initializeCell(actorList.cell);

    // If this now exists, send it the data
    if (cellsInitialized.count(mapIndex) > 0)
    {
        cellsInitialized[mapIndex]->readAnimPlay(actorList);
    }
}

void CellController::readStatsDynamic(ActorList& actorList)
{
    std::string mapIndex = actorList.cell.getDescription();

    initializeCell(actorList.cell);

    // If this now exists, send it the data
    if (cellsInitialized.count(mapIndex) > 0)
    {
        cellsInitialized[mapIndex]->readStatsDynamic(actorList);
    }
}

void CellController::readEquipment(ActorList& actorList)
{
    std::string mapIndex = actorList.cell.getDescription();

    initializeCell(actorList.cell);

    // If this now exists, send it the data
    if (cellsInitialized.count(mapIndex) > 0)
    {
        cellsInitialized[mapIndex]->readEquipment(actorList);
    }
}

void CellController::readSpeech(ActorList& actorList)
{
    std::string mapIndex = actorList.cell.getDescription();

    initializeCell(actorList.cell);

    // If this now exists, send it the data
    if (cellsInitialized.count(mapIndex) > 0)
    {
        cellsInitialized[mapIndex]->readSpeech(actorList);
    }
}

void CellController::readAttack(ActorList& actorList)
{
    std::string mapIndex = actorList.cell.getDescription();

    initializeCell(actorList.cell);

    // If this now exists, send it the data
    if (cellsInitialized.count(mapIndex) > 0)
    {
        cellsInitialized[mapIndex]->readAttack(actorList);
    }
}

void CellController::readCellChange(ActorList& actorList)
{
    std::string mapIndex = actorList.cell.getDescription();

    initializeCell(actorList.cell);

    // If this now exists, send it the data
    if (cellsInitialized.count(mapIndex) > 0)
    {
        cellsInitialized[mapIndex]->readCellChange(actorList);
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

bool CellController::isLocalActor(MWWorld::Ptr ptr)
{
    if (ptr.mRef == NULL)
        return false;

    std::string mapIndex = generateMapIndex(ptr);

    return (localActorsToCells.count(mapIndex) > 0);
}

bool CellController::isLocalActor(int refNumIndex, int mpNum)
{
    std::string mapIndex = generateMapIndex(refNumIndex, mpNum);
    return (localActorsToCells.count(mapIndex) > 0);
}

LocalActor *CellController::getLocalActor(MWWorld::Ptr ptr)
{
    std::string actorIndex = generateMapIndex(ptr);
    std::string cellIndex = localActorsToCells.at(actorIndex);

    return cellsInitialized.at(cellIndex)->getLocalActor(actorIndex);
}

LocalActor *CellController::getLocalActor(int refNumIndex, int mpNum)
{
    std::string actorIndex = generateMapIndex(refNumIndex, mpNum);
    std::string cellIndex = localActorsToCells.at(actorIndex);

    return cellsInitialized.at(cellIndex)->getLocalActor(actorIndex);
}

void CellController::setDedicatedActorRecord(std::string actorIndex, std::string cellIndex)
{
    dedicatedActorsToCells[actorIndex] = cellIndex;
}

void CellController::removeDedicatedActorRecord(std::string actorIndex)
{
    dedicatedActorsToCells.erase(actorIndex);
}

bool CellController::isDedicatedActor(int refNumIndex, int mpNum)
{
    std::string mapIndex = generateMapIndex(refNumIndex, mpNum);
    return (dedicatedActorsToCells.count(mapIndex) > 0);
}

bool CellController::isDedicatedActor(MWWorld::Ptr ptr)
{
    if (ptr.mRef == NULL)
        return false;

    std::string mapIndex = generateMapIndex(ptr);

    return (dedicatedActorsToCells.count(mapIndex) > 0);
}

DedicatedActor *CellController::getDedicatedActor(MWWorld::Ptr ptr)
{
    std::string actorIndex = generateMapIndex(ptr);
    std::string cellIndex = dedicatedActorsToCells.at(actorIndex);

    return cellsInitialized.at(cellIndex)->getDedicatedActor(actorIndex);
}

DedicatedActor *CellController::getDedicatedActor(int refNumIndex, int mpNum)
{
    std::string actorIndex = generateMapIndex(refNumIndex, mpNum);
    std::string cellIndex = dedicatedActorsToCells.at(actorIndex);

    return cellsInitialized.at(cellIndex)->getDedicatedActor(actorIndex);
}

std::string CellController::generateMapIndex(int refNumIndex, int mpNum)
{
    std::string mapIndex = "";
    mapIndex = Utils::toString(refNumIndex) + "-" + Utils::toString(mpNum);
    return mapIndex;
}

std::string CellController::generateMapIndex(MWWorld::Ptr ptr)
{
    return generateMapIndex(ptr.getCellRef().getRefNum().mIndex, ptr.getCellRef().getMpNum());
}

std::string CellController::generateMapIndex(BaseActor baseActor)
{
    return generateMapIndex(baseActor.refNumIndex, baseActor.mpNum);
}

bool CellController::hasLocalAuthority(const ESM::Cell& cell)
{
    if (isInitializedCell(cell) && isActiveWorldCell(cell))
    {
        return getCell(cell)->hasLocalAuthority();
    }

    return false;
}

bool CellController::isInitializedCell(const ESM::Cell& cell)
{
    return (cellsInitialized.count(cell.getDescription()) > 0);
}

bool CellController::isActiveWorldCell(const ESM::Cell& cell)
{
    MWWorld::CellStore *cellStore = getCellStore(cell);

    if (!cellStore) return false;

    return MWBase::Environment::get().getWorld()->isCellActive(cellStore);
}

Cell *CellController::getCell(const ESM::Cell& cell)
{
    return cellsInitialized.at(cell.getDescription());
}

MWWorld::CellStore *CellController::getCellStore(const ESM::Cell& cell)
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

bool CellController::isSameCell(const ESM::Cell& cell, const ESM::Cell& otherCell)
{
    if (cell.isExterior() && otherCell.isExterior())
    {
        if (cell.mData.mX == otherCell.mData.mX && cell.mData.mY == otherCell.mData.mY)
            return true;
    }
    else if (Misc::StringUtils::ciEqual(cell.mName, otherCell.mName))
        return true;

    return false;
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

        /*if (::Misc::StringUtils::ciEqual(name, "gold_001"))
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

int CellController::getCellSize() const
{
    return 8192;
}
