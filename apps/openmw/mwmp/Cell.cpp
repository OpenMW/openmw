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
            worldObject.drawState = actor->drawState;

            worldObject.headPitch = actor->headPitch;
            worldObject.headYaw = actor->headYaw;

            worldObject.hasAnimation = actor->hasAnimation;
            worldObject.hasAnimStates = actor->hasAnimStates;
            worldObject.hasMovement = actor->hasMovement;

            if (actor->hasAnimation)
            {
                worldObject.animation = actor->animation;
            }

            if (actor->hasAnimStates)
            {
                worldObject.animStates = actor->animStates;
            }

            if (actor->hasMovement)
            {
                worldObject.movement = actor->movement;
            }

            actor->hasAnimation = false;
            actor->hasAnimStates = false;
            actor->hasMovement = false;
            worldEvent->addObject(worldObject);

            ++it;
        }
    }

    Main::get().getNetworking()->getWorldPacket(ID_ACTOR_FRAME)->setEvent(worldEvent);
    Main::get().getNetworking()->getWorldPacket(ID_ACTOR_FRAME)->Send();
}

void Cell::updateDedicated(float dt)
{
    if (dedicatedActors.empty()) return;

    for (std::map<std::string, DedicatedActor *>::iterator it = dedicatedActors.begin(); it != dedicatedActors.end(); ++it)
    {
        DedicatedActor *actor = it->second;

        actor->update(dt);
    }
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

            Main::get().getCellController()->setDedicatedActorRecord(mapIndex, getDescription());

            LOG_APPEND(Log::LOG_INFO, "- Initialized DedicatedActor %s", mapIndex.c_str());
        }

        // If this now exists, set its details
        if (dedicatedActors.count(mapIndex) > 0)
        {
            DedicatedActor *actor = dedicatedActors[mapIndex];
            actor->position = worldObject.pos;
            actor->drawState = worldObject.drawState;

            actor->hasAnimation = worldObject.hasAnimation;
            actor->hasAnimStates = worldObject.hasAnimStates;
            actor->hasMovement = worldObject.hasMovement;

            if (actor->hasAnimation)
            {
                actor->animation = worldObject.animation;
            }

            if (actor->hasAnimStates)
            {
                actor->animStates = worldObject.animStates;
            }

            if (actor->hasMovement)
            {
                actor->movement = worldObject.movement;
            }
        }
    }
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

void Cell::uninitializeDedicatedActors()
{
    for (std::map<std::string, DedicatedActor *>::iterator it = dedicatedActors.begin(); it != dedicatedActors.end(); ++it)
    {
        Main::get().getCellController()->removeDedicatedActorRecord(it->first);
    }

    dedicatedActors.clear();
}

LocalActor *Cell::getLocalActor(std::string actorIndex)
{
    return localActors.at(actorIndex);
}

DedicatedActor *Cell::getDedicatedActor(std::string actorIndex)
{
    return dedicatedActors.at(actorIndex);
}

MWWorld::CellStore *Cell::getCellStore()
{
    return store;
}

std::string Cell::getDescription()
{
    return store->getCell()->getDescription();
}
