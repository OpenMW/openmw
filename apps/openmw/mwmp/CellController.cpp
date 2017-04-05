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

std::map<std::string, LocalActor *> CellController::localActors;
std::map<std::string, DedicatedActor *> CellController::dedicatedActors;

mwmp::CellController::CellController()
{

}

mwmp::CellController::~CellController()
{

}

void CellController::update()
{
    for (std::map<std::string, LocalActor *>::iterator it = localActors.begin(); it != localActors.end();)
    {
        LocalActor *actor = it->second;

        if (!MWBase::Environment::get().getWorld()->isCellActive(getCell(actor->cell)))
        {
            localActors.erase(it++);
        }
        else
        {
            printf("Updating %s\n", it->first.c_str());
            ++it;
        }
    }
}

void CellController::initializeLocalActors(const ESM::Cell& cell)
{
    MWWorld::CellStore *cellStore = getCell(cell);

    if (!cellStore) return;

    MWWorld::CellRefList<ESM::NPC> *npcList = cellStore->getNpcs();

    for (typename MWWorld::CellRefList<ESM::NPC>::List::iterator listIter(npcList->mList.begin());
        listIter != npcList->mList.end(); ++listIter)
    {
        MWWorld::Ptr ptr(&*listIter, 0);

        std::string mapIndex = generateMapIndex(ptr);
        localActors[mapIndex] = new LocalActor();
        localActors[mapIndex]->cell = cell;
        printf("Initialized local actor %s\n", mapIndex.c_str());
    }
}

std::string CellController::generateMapIndex(MWWorld::Ptr ptr)
{
    std::string mapIndex = "";
    mapIndex += ptr.getCellRef().getRefId();
    mapIndex += "-" + std::to_string(ptr.getCellRef().getRefNum().mIndex);
    mapIndex += "-" + std::to_string(ptr.getCellRef().getMpNum());
    return mapIndex;
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
