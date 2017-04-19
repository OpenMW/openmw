#include <components/esm/cellid.hpp>
#include <components/openmw-mp/Log.hpp>

#include "../mwworld/worldimp.hpp"

#include "Cell.hpp"
#include "Main.hpp"
#include "Networking.hpp"
#include "LocalPlayer.hpp"
#include "CellController.hpp"
#include "MechanicsHelper.hpp"

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

void Cell::updateLocal(bool forceUpdate)
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
            
            Main::get().getCellController()->removeLocalActorRecord(it->first);
            localActors.erase(it++);
        }
        else
        {
            if (actor->getPtr().getRefData().isEnabled())
                actor->update(forceUpdate);

            ++it;
        }
    }

    actorList->sendPositionActors();
    actorList->sendAnimFlagsActors();
    actorList->sendAnimPlayActors();
    actorList->sendSpeechActors();
    actorList->sendStatsDynamicActors();
    actorList->sendAttackActors();
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

void Cell::readPositions(ActorList& actorList)
{
    initializeDedicatedActors(actorList);
    
    BaseActor baseActor;

    for (unsigned int i = 0; i < actorList.count; i++)
    {
        baseActor = actorList.baseActors.at(i);
        std::string mapIndex = Main::get().getCellController()->generateMapIndex(baseActor);

        if (dedicatedActors.count(mapIndex) > 0)
        {
            DedicatedActor *actor = dedicatedActors[mapIndex];
            actor->position = baseActor.position;
            actor->direction = baseActor.direction;
        }
    }
}

void Cell::readAnimFlags(ActorList& actorList)
{
    initializeDedicatedActors(actorList);

    BaseActor baseActor;

    for (unsigned int i = 0; i < actorList.count; i++)
    {
        baseActor = actorList.baseActors.at(i);
        std::string mapIndex = Main::get().getCellController()->generateMapIndex(baseActor);

        if (dedicatedActors.count(mapIndex) > 0)
        {
            DedicatedActor *actor = dedicatedActors[mapIndex];
            actor->movementFlags = baseActor.movementFlags;
            actor->drawState = baseActor.drawState;
            actor->isFlying = baseActor.isFlying;
        }
    }
}

void Cell::readAnimPlay(ActorList& actorList)
{
    initializeDedicatedActors(actorList);

    BaseActor baseActor;

    for (unsigned int i = 0; i < actorList.count; i++)
    {
        baseActor = actorList.baseActors.at(i);
        std::string mapIndex = Main::get().getCellController()->generateMapIndex(baseActor);

        if (dedicatedActors.count(mapIndex) > 0)
        {
            DedicatedActor *actor = dedicatedActors[mapIndex];
            actor->animation.groupname = baseActor.animation.groupname;
            actor->animation.mode = baseActor.animation.mode;
            actor->animation.count = baseActor.animation.count;
            actor->animation.persist = baseActor.animation.persist;
        }
    }
}

void Cell::readStatsDynamic(ActorList& actorList)
{
    initializeDedicatedActors(actorList);

    BaseActor baseActor;

    for (unsigned int i = 0; i < actorList.count; i++)
    {
        baseActor = actorList.baseActors.at(i);
        std::string mapIndex = Main::get().getCellController()->generateMapIndex(baseActor);

        if (dedicatedActors.count(mapIndex) > 0)
        {
            DedicatedActor *actor = dedicatedActors[mapIndex];
            actor->creatureStats = baseActor.creatureStats;
        }
    }
}

void Cell::readSpeech(ActorList& actorList)
{
    initializeDedicatedActors(actorList);

    BaseActor baseActor;

    for (unsigned int i = 0; i < actorList.count; i++)
    {
        baseActor = actorList.baseActors.at(i);
        std::string mapIndex = Main::get().getCellController()->generateMapIndex(baseActor);

        if (dedicatedActors.count(mapIndex) > 0)
        {
            DedicatedActor *actor = dedicatedActors[mapIndex];
            actor->response = baseActor.response;
            actor->sound = baseActor.sound;
        }
    }
}

void Cell::readAttack(ActorList& actorList)
{
    initializeDedicatedActors(actorList);

    BaseActor baseActor;

    for (unsigned int i = 0; i < actorList.count; i++)
    {
        baseActor = actorList.baseActors.at(i);
        std::string mapIndex = Main::get().getCellController()->generateMapIndex(baseActor);

        if (dedicatedActors.count(mapIndex) > 0)
        {
            DedicatedActor *actor = dedicatedActors[mapIndex];
            actor->attack = baseActor.attack;
            mwmp::Main::get().getMechanicsHelper()->processAttack(actor->attack, actor->getPtr());
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
        actor->setPtr(ptr);

        std::string mapIndex = Main::get().getCellController()->generateMapIndex(ptr);
        localActors[mapIndex] = actor;

        Main::get().getCellController()->setLocalActorRecord(mapIndex, getDescription());

        LOG_APPEND(Log::LOG_INFO, "- Initialized LocalActor %s", mapIndex.c_str());
    }
}

void Cell::initializeDedicatedActors(ActorList& actorList)
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
    }
}

void Cell::uninitializeLocalActors()
{
    for (std::map<std::string, LocalActor *>::iterator it = localActors.begin(); it != localActors.end(); ++it)
    {
        LocalActor *actor = it->second;

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
