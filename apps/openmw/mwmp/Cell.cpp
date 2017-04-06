#include "../mwworld/worldimp.hpp"
#include <components/esm/cellid.hpp>
#include <components/openmw-mp/Log.hpp>
#include <components/openmw-mp/Utils.hpp>

#include "Cell.hpp"
#include "Main.hpp"
#include "Networking.hpp"
#include "LocalPlayer.hpp"
using namespace mwmp;

mwmp::Cell::Cell(MWWorld::CellStore* cellStore)
{
    store = cellStore;

    std::map<std::string, LocalActor *> localActors;
    std::map<std::string, DedicatedActor *> dedicatedActors;
}

mwmp::Cell::~Cell()
{

}

void Cell::updateLocal()
{
    if (localActors.empty()) return;

    mwmp::WorldEvent *worldEvent = mwmp::Main::get().getNetworking()->getWorldEvent();
    worldEvent->reset();
    worldEvent->cell = *store->getCell();

    for (std::map<std::string, LocalActor *>::iterator it = localActors.begin(); it != localActors.end();)
    {
        LocalActor *actor = it->second;

        // TODO:: Make sure this condition actually works
        if (actor->getPtr().getCell() && actor->getPtr().getCell() != store)
        {
            LOG_APPEND(Log::LOG_INFO, "- Removing LocalActor %s which is no longer in this cell", it->first.c_str());
            actor->getPtr().getBase()->isLocalActor = false;
            localActors.erase(it++);
        }
        else
        {
            //LOG_APPEND(Log::LOG_VERBOSE, "- Updating LocalActor %s", it->first.c_str());
            //actor->update();
            MWWorld::Ptr ptr = actor->getPtr();

            mwmp::WorldObject worldObject;
            worldObject.refId = ptr.getCellRef().getRefId();
            worldObject.refNumIndex = ptr.getCellRef().getRefNum().mIndex;
            worldObject.mpNum = ptr.getCellRef().getMpNum();
            worldObject.pos = ptr.getRefData().getPosition();

            worldEvent->addObject(worldObject);

            ++it;
        }
    }

    mwmp::Main::get().getNetworking()->getWorldPacket(ID_ACTOR_FRAME)->setEvent(worldEvent);
    mwmp::Main::get().getNetworking()->getWorldPacket(ID_ACTOR_FRAME)->Send();
}

void Cell::initializeLocalActors()
{
    ESM::Cell esmCell = *store->getCell();
    MWWorld::CellRefList<ESM::NPC> *npcList = store->getNpcs();

    for (typename MWWorld::CellRefList<ESM::NPC>::List::iterator listIter(npcList->mList.begin());
        listIter != npcList->mList.end(); ++listIter)
    {
        MWWorld::Ptr ptr(&*listIter, 0);

        std::string mapIndex = generateMapIndex(ptr);
        localActors[mapIndex] = new LocalActor();
        localActors[mapIndex]->cell = esmCell;
        ptr.getBase()->isLocalActor = true;
        localActors[mapIndex]->setPtr(ptr);
        LOG_APPEND(Log::LOG_INFO, "- Initialized LocalActor %s", mapIndex.c_str());
    }
}

void Cell::uninitializeLocalActors()
{
    for (std::map<std::string, LocalActor *>::iterator it = localActors.begin(); it != localActors.end(); ++it)
    {
        LocalActor *actor = it->second;
        actor->getPtr().getBase()->isLocalActor = false;
    }

    localActors.clear();
}

void Cell::readCellFrame(mwmp::WorldEvent& worldEvent)
{
    WorldObject worldObject;

    for (unsigned int i = 0; i < worldEvent.objectChanges.count; i++)
    {
        worldObject = worldEvent.objectChanges.objects.at(i);
        std::string mapIndex = generateMapIndex(worldObject);

        // If this key doesn't exist, create it
        if (dedicatedActors.count(mapIndex) == 0)
        {
            MWWorld::Ptr ptrFound = store->searchExact(worldObject.refId, worldObject.refNumIndex, worldObject.mpNum);

            if (ptrFound)
            {
                dedicatedActors[mapIndex] = new DedicatedActor();
                dedicatedActors[mapIndex]->cell = worldEvent.cell;
                dedicatedActors[mapIndex]->setPtr(ptrFound);
                LOG_APPEND(Log::LOG_INFO, "- Initialized DedicatedActor %s", mapIndex.c_str());
            }
        }

        // If this now exists, set its details
        if (dedicatedActors.count(mapIndex) > 0)
        {
            DedicatedActor *actor = dedicatedActors[mapIndex];
            actor->position = worldObject.pos;
            actor->move();
        }
    }
}

std::string Cell::generateMapIndex(MWWorld::Ptr ptr)
{
    std::string mapIndex = "";
    mapIndex += ptr.getCellRef().getRefId();
    mapIndex += "-" + Utils::toString(ptr.getCellRef().getRefNum().mIndex);
    mapIndex += "-" + Utils::toString(ptr.getCellRef().getMpNum());
    return mapIndex;
}

std::string Cell::generateMapIndex(mwmp::WorldObject object)
{
    std::string mapIndex = "";
    mapIndex += object.refId;
    mapIndex += "-" + Utils::toString(object.refNumIndex);
    mapIndex += "-" + Utils::toString(object.mpNum);
    return mapIndex;
}

MWWorld::CellStore *mwmp::Cell::getCellStore()
{
    return store;
}

std::string mwmp::Cell::getDescription()
{
    return store->getCell()->getDescription();
}
