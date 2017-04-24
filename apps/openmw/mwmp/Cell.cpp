#include <components/esm/cellid.hpp>
#include <components/openmw-mp/Log.hpp>

#include "../mwworld/worldimp.hpp"

#include "Cell.hpp"
#include "Main.hpp"
#include "Networking.hpp"
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

        MWWorld::CellStore *newStore = actor->getPtr().getCell();

        if (newStore != store)
        {
            actor->updateCell();

            LOG_APPEND(Log::LOG_INFO, "- Removing LocalActor %s which is no longer in this cell", it->first.c_str());
            
            Main::get().getCellController()->removeLocalActorRecord(it->first);

            // If the cell this actor has moved to is active, initialize them in it
            if (Main::get().getCellController()->isInitializedCell(*newStore->getCell()))
                Main::get().getCellController()->getCell(*newStore->getCell())->initializeLocalActor(actor->getPtr());

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
    actorList->sendCellChangeActors();
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

void Cell::readCellChange(ActorList& actorList)
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
            actor->cell = baseActor.cell;
            actor->position = baseActor.position;

            LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Server says DedicatedActor %s, %i, %i moved to %s",
                actor->refId.c_str(), actor->refNumIndex, actor->mpNum, actor->cell.getDescription().c_str());

            MWWorld::CellStore *newStore = Main::get().getCellController()->getCellStore(actor->cell);
            actor->setCell(newStore);

            Main::get().getCellController()->removeDedicatedActorRecord(mapIndex);
            
            // If the cell this actor has moved to is active, initialize them in it
            if (Main::get().getCellController()->isInitializedCell(actor->cell))
                Main::get().getCellController()->getCell(actor->cell)->initializeDedicatedActor(actor->getPtr());

            dedicatedActors.erase(mapIndex);
        }
    }
}

void Cell::initializeLocalActor(const MWWorld::Ptr& ptr)
{
    LocalActor *actor = new LocalActor();
    actor->cell = *store->getCell();
    actor->setPtr(ptr);

    std::string mapIndex = Main::get().getCellController()->generateMapIndex(ptr);
    localActors[mapIndex] = actor;

    Main::get().getCellController()->setLocalActorRecord(mapIndex, getDescription());

    LOG_APPEND(Log::LOG_INFO, "- Initialized LocalActor %s in %s", mapIndex.c_str(), getDescription().c_str());
}

void Cell::initializeLocalActors()
{
    MWWorld::CellRefList<ESM::NPC> *npcList = store->getNpcs();

    for (typename MWWorld::CellRefList<ESM::NPC>::List::iterator listIter(npcList->mList.begin());
        listIter != npcList->mList.end(); ++listIter)
    {
        MWWorld::Ptr ptr(&*listIter, store);

        initializeLocalActor(ptr);
    }

    MWWorld::CellRefList<ESM::Creature> *creatureList = store->getCreatures();

    for (typename MWWorld::CellRefList<ESM::Creature>::List::iterator listIter(creatureList->mList.begin());
        listIter != creatureList->mList.end(); ++listIter)
    {
        MWWorld::Ptr ptr(&*listIter, store);

        initializeLocalActor(ptr);
    }
}

void Cell::initializeDedicatedActor(const MWWorld::Ptr& ptr)
{
    DedicatedActor *actor = new DedicatedActor();
    actor->cell = *store->getCell();
    actor->setPtr(ptr);

    std::string mapIndex = Main::get().getCellController()->generateMapIndex(ptr);
    dedicatedActors[mapIndex] = actor;

    Main::get().getCellController()->setDedicatedActorRecord(mapIndex, getDescription());

    LOG_APPEND(Log::LOG_INFO, "- Initialized DedicatedActor %s in %s", mapIndex.c_str(), getDescription().c_str());
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

            initializeDedicatedActor(ptrFound);
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
