#include <components/esm/cellid.hpp>
#include <components/openmw-mp/Log.hpp>

#include "../mwworld/worldimp.hpp"

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

    ActorList *actorList = mwmp::Main::get().getNetworking()->getActorList();
    actorList->reset();

    actorList->cell = *store->getCell();

    for (std::map<std::string, LocalActor *>::iterator it = localActors.begin(); it != localActors.end();)
    {
        LocalActor *actor = it->second;

        // TODO:: Make sure this condition actually works
        if (actor->getPtr().getCell() != store)
        {
            LOG_APPEND(Log::LOG_INFO, "- Removing LocalActor %s which is no longer in this cell", it->first.c_str());
            actor->getPtr().getBase()->isLocalActor = false;
            localActors.erase(it++);
        }
        else
        {
            //LOG_APPEND(Log::LOG_VERBOSE, "- Updating LocalActor %s", it->first.c_str());
            actor->update();

            actorList->addActor(*actor);
            actor->hasAnimation = false;
            actor->hasAnimStates = false;
            actor->hasMovement = false;

            ++it;
        }
    }

    Main::get().getNetworking()->getActorPacket(ID_ACTOR_FRAME)->setActorList(actorList);
    Main::get().getNetworking()->getActorPacket(ID_ACTOR_FRAME)->Send();
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

void Cell::readCellFrame(ActorList& actorList)
{
    BaseActor baseActor;

    for (unsigned int i = 0; i < actorList.count; i++)
    {
        baseActor = actorList.baseActors.at(i);
        std::string mapIndex = Main::get().getCellController()->generateMapIndex(baseActor);

        // If this key doesn't exist, create it
        if (dedicatedActors.count(mapIndex) == 0)
        {
            MWWorld::Ptr ptrFound = store->searchExact(baseActor.refId, baseActor.refNumIndex, baseActor.mpNum);

            if (!ptrFound) return;

            DedicatedActor *actor = new DedicatedActor();
            actor->cell = actorList.cell;
            actor->setPtr(ptrFound);
            dedicatedActors[mapIndex] = actor;

            Main::get().getCellController()->setDedicatedActorRecord(mapIndex, getDescription());

            LOG_APPEND(Log::LOG_INFO, "- Initialized DedicatedActor %s", mapIndex.c_str());
        }

        // If this now exists, set its details
        if (dedicatedActors.count(mapIndex) > 0)
        {
            DedicatedActor *actor = dedicatedActors[mapIndex];
            actor->position = baseActor.position;
            actor->drawState = baseActor.drawState;

            actor->headPitch = baseActor.headPitch;
            actor->headYaw = baseActor.headYaw;

            actor->hasAnimation = baseActor.hasAnimation;
            actor->hasAnimStates = baseActor.hasAnimStates;
            actor->hasMovement = baseActor.hasMovement;

            if (actor->hasAnimation)
            {
                actor->animation = baseActor.animation;
            }

            if (actor->hasAnimStates)
            {
                actor->animStates = baseActor.animStates;
            }

            if (actor->hasMovement)
            {
                actor->movement = baseActor.movement;
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
        MWWorld::Ptr ptr(&*listIter, store);

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
