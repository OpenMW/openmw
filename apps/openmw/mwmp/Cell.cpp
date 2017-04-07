#include "../mwworld/worldimp.hpp"
#include <components/esm/cellid.hpp>
#include <components/openmw-mp/Log.hpp>

#include "Cell.hpp"
#include "CellController.hpp"
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

    WorldEvent *worldEvent = mwmp::Main::get().getNetworking()->getWorldEvent();
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
            actor->update();
            MWWorld::Ptr ptr = actor->getPtr();

            WorldObject worldObject;
            worldObject.refId = ptr.getCellRef().getRefId();
            worldObject.refNumIndex = ptr.getCellRef().getRefNum().mIndex;
            worldObject.mpNum = ptr.getCellRef().getMpNum();
            worldObject.pos = actor->position;
            worldObject.direction = actor->direction;
            worldObject.drawState = actor->drawState;
            worldObject.movementFlags = actor->movementFlags;
            worldObject.headPitch = actor->headPitch;
            worldObject.headYaw = actor->headYaw;

            worldEvent->addObject(worldObject);

            ++it;
        }
    }

    Main::get().getNetworking()->getWorldPacket(ID_ACTOR_FRAME)->setEvent(worldEvent);
    Main::get().getNetworking()->getWorldPacket(ID_ACTOR_FRAME)->Send();
}

void Cell::initializeLocalActors()
{
    ESM::Cell esmCell = *store->getCell();
    MWWorld::CellRefList<ESM::NPC> *npcList = store->getNpcs();

    for (typename MWWorld::CellRefList<ESM::NPC>::List::iterator listIter(npcList->mList.begin());
        listIter != npcList->mList.end(); ++listIter)
    {
        MWWorld::Ptr ptr(&*listIter, 0);

        LocalActor *actor = new LocalActor();
        actor->cell = esmCell;
        ptr.getBase()->isLocalActor = true;
        actor->setPtr(ptr);

        std::string mapIndex = Main::get().getCellController()->generateMapIndex(ptr);
        localActors[mapIndex] = actor;

        Main::get().getCellController()->setLocalActorRecord(mapIndex, getDescription());
        
        LOG_APPEND(Log::LOG_INFO, "- Initialized LocalActor %s", mapIndex.c_str());
    }
}

void Cell::uninitializeLocalActors()
{
    for (std::map<std::string, LocalActor *>::iterator it = localActors.begin(); it != localActors.end(); ++it)
    {
        LocalActor *actor = it->second;
        actor->getPtr().getBase()->isLocalActor = false;

        Main::get().getCellController()->removeLocalActorRecord(it->first);
    }

    localActors.clear();
}

void Cell::readCellFrame(WorldEvent& worldEvent)
{
    WorldObject worldObject;

    for (unsigned int i = 0; i < worldEvent.objectChanges.count; i++)
    {
        worldObject = worldEvent.objectChanges.objects.at(i);
        std::string mapIndex = Main::get().getCellController()->generateMapIndex(worldObject);

        // If this key doesn't exist, create it
        if (dedicatedActors.count(mapIndex) == 0)
        {
            MWWorld::Ptr ptrFound = store->searchExact(worldObject.refId, worldObject.refNumIndex, worldObject.mpNum);

            if (!ptrFound) return;

            DedicatedActor *actor = new DedicatedActor();
            actor->cell = worldEvent.cell;
            actor->setPtr(ptrFound);
            dedicatedActors[mapIndex] = actor;

            LOG_APPEND(Log::LOG_INFO, "- Initialized DedicatedActor %s", mapIndex.c_str());
        }

        // If this now exists, set its details
        if (dedicatedActors.count(mapIndex) > 0)
        {
            DedicatedActor *actor = dedicatedActors[mapIndex];
            actor->position = worldObject.pos;
            actor->direction = worldObject.direction;
            actor->drawState = worldObject.drawState;
            actor->movementFlags = worldObject.movementFlags;
            actor->headPitch = worldObject.headPitch;
            actor->headYaw = worldObject.headYaw;
            actor->move();
            actor->setDrawState();
            actor->setMovementFlags();
            actor->setAnimation();
        }
    }
}

LocalActor *Cell::getLocalActor(std::string actorIndex)
{
    return localActors.at(actorIndex);
}

MWWorld::CellStore *Cell::getCellStore()
{
    return store;
}

std::string Cell::getDescription()
{
    return store->getCell()->getDescription();
}
