#include <components/esm/cellid.hpp>
#include <components/openmw-mp/Log.hpp>

#include "../mwbase/environment.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/livecellref.hpp"
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
    shouldInitializeActors = false;

    std::map<std::string, LocalActor *> localActors;
    std::map<std::string, DedicatedActor *> dedicatedActors;

    updateTimer = 0;
}

Cell::~Cell()
{

}

void Cell::updateLocal(bool forceUpdate)
{
    if (localActors.empty())
        return;

    const float timeoutSec = 0.025;

    if (!forceUpdate && (updateTimer += MWBase::Environment::get().getFrameDuration()) < timeoutSec)
        return;
    else
        updateTimer = 0;

    CellController *cellController = Main::get().getCellController();
    ActorList *actorList = mwmp::Main::get().getNetworking()->getActorList();
    actorList->reset();

    actorList->cell = *store->getCell();

    for (auto it = localActors.begin(); it != localActors.end();)
    {
        LocalActor *actor = it->second;

        MWWorld::CellStore *newStore = actor->getPtr().getCell();

        if (newStore != store)
        {
            actor->updateCell();
            std::string mapIndex = it->first;

            // If the cell this actor has moved to is under our authority, move them to it
            if (cellController->hasLocalAuthority(actor->cell))
            {
                LOG_APPEND(Log::LOG_INFO, "- Moving LocalActor %s to our authority in %s", mapIndex.c_str(), actor->cell.getDescription().c_str());
                Cell *newCell = cellController->getCell(actor->cell);
                newCell->localActors[mapIndex] = actor;
                cellController->setLocalActorRecord(mapIndex, newCell->getDescription());
            }
            else
            {
                LOG_APPEND(Log::LOG_INFO, "- Deleting LocalActor %s which is no longer under our authority", mapIndex.c_str(), getDescription().c_str());
                cellController->removeLocalActorRecord(mapIndex);
                delete actor;
            }

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
    actorList->sendEquipmentActors();
    actorList->sendAttackActors();
    actorList->sendCellChangeActors();
}

void Cell::updateDedicated(float dt)
{
    if (dedicatedActors.empty()) return;
    
    for (auto &actor : dedicatedActors)
        actor.second->update(dt);

    // Are we the authority over this cell? If so, uninitialize DedicatedActors
    // after the above update
    if (hasLocalAuthority())
        uninitializeDedicatedActors();
}

void Cell::readPositions(ActorList& actorList)
{
    initializeDedicatedActors(actorList);
    
    for (const auto &baseActor : actorList.baseActors)
    {
        std::string mapIndex = Main::get().getCellController()->generateMapIndex(baseActor);

        if (dedicatedActors.count(mapIndex) > 0)
        {
            DedicatedActor *actor = dedicatedActors[mapIndex];
            actor->position = baseActor.position;
            actor->direction = baseActor.direction;

            if (!actor->hasPositionData)
            {
                actor->hasPositionData = true;

                // If this is our first packet about this actor's position, force an update
                // now instead of waiting for its frame
                //
                // That way, if this actor is about to become a LocalActor, initial data about it
                // received from the server still gets set
                actor->setPosition();
            }
        }
    }
}

void Cell::readAnimFlags(ActorList& actorList)
{
    for (const auto &baseActor : actorList.baseActors)
    {
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
    for (const auto &baseActor : actorList.baseActors)
    {
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

    for (const auto &baseActor : actorList.baseActors)
    {
        std::string mapIndex = Main::get().getCellController()->generateMapIndex(baseActor);

        if (dedicatedActors.count(mapIndex) > 0)
        {
            DedicatedActor *actor = dedicatedActors[mapIndex];
            actor->creatureStats = baseActor.creatureStats;

            if (!actor->hasStatsDynamicData)
            {
                actor->hasStatsDynamicData = true;

                // If this is our first packet about this actor's dynamic stats, force an update
                // now instead of waiting for its frame
                //
                // That way, if this actor is about to become a LocalActor, initial data about it
                // received from the server still gets set
                actor->setStatsDynamic();

                // Actors loaded as dead from the server need special handling to skip their death animations
                // and disable their collision
                if (actor->creatureStats.mDynamic[0].mCurrent < 1)
                {
                    actor->getPtr().getClass().getCreatureStats(actor->getPtr()).setDeathAnimationFinished(true);
                    MWBase::Environment::get().getWorld()->enableActorCollision(actor->getPtr(), false);
                }
            }
        }
    }
}

void Cell::readEquipment(ActorList& actorList)
{
    initializeDedicatedActors(actorList);

    for (const auto &baseActor : actorList.baseActors)
    {
        std::string mapIndex = Main::get().getCellController()->generateMapIndex(baseActor);

        if (dedicatedActors.count(mapIndex) > 0)
        {
            DedicatedActor *actor = dedicatedActors[mapIndex];

            for (int slot = 0; slot < 19; ++slot)
                actor->equipedItems[slot] = baseActor.equipedItems[slot];

            actor->setEquipment();
        }
    }
}

void Cell::readSpeech(ActorList& actorList)
{
    for (const auto &baseActor : actorList.baseActors)
    {
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
    for (const auto &baseActor : actorList.baseActors)
    {
        std::string mapIndex = Main::get().getCellController()->generateMapIndex(baseActor);

        if (dedicatedActors.count(mapIndex) > 0)
        {
            DedicatedActor *actor = dedicatedActors[mapIndex];
            actor->attack = baseActor.attack;

            // Set the correct drawState here if we've somehow we've missed a previous
            // AnimFlags packet
            if (actor->attack.type == mwmp::Attack::MELEE && actor->drawState != 1)
            {
                actor->drawState = 1;
                actor->setAnimFlags();
            }
            else if (actor->attack.type == mwmp::Attack::MAGIC && actor->drawState != 2)
            {
                actor->drawState = 2;
                actor->setAnimFlags();
            }

            MechanicsHelper::processAttack(actor->attack, actor->getPtr());
        }
    }
}

void Cell::readCellChange(ActorList& actorList)
{
    initializeDedicatedActors(actorList);

    CellController *cellController = Main::get().getCellController();

    for (const auto &baseActor : actorList.baseActors)
    {
        std::string mapIndex = Main::get().getCellController()->generateMapIndex(baseActor);

        if (dedicatedActors.count(mapIndex) > 0)
        {
            DedicatedActor *dedicatedActor = dedicatedActors[mapIndex];
            dedicatedActor->cell = baseActor.cell;
            dedicatedActor->position = baseActor.position;
            dedicatedActor->direction = baseActor.direction;

            LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Server says DedicatedActor %s moved to %s", mapIndex.c_str(), dedicatedActor->cell.getDescription().c_str());

            MWWorld::CellStore *newStore = cellController->getCellStore(dedicatedActor->cell);
            dedicatedActor->setCell(newStore);

            // If the cell this actor has moved to is active and not under our authority, move them to it
            if (cellController->isActiveWorldCell(dedicatedActor->cell) && !cellController->hasLocalAuthority(dedicatedActor->cell))
            {
                LOG_APPEND(Log::LOG_INFO, "- Moving DedicatedActor %s to our active cell %s", mapIndex.c_str(), dedicatedActor->cell.getDescription().c_str());
                cellController->initializeCell(dedicatedActor->cell);
                Cell *newCell = cellController->getCell(dedicatedActor->cell);
                newCell->dedicatedActors[mapIndex] = dedicatedActor;
                cellController->setDedicatedActorRecord(mapIndex, newCell->getDescription());
            }
            else
            {
                if (cellController->hasLocalAuthority(dedicatedActor->cell))
                {
                    LOG_APPEND(Log::LOG_INFO, "- Creating new LocalActor based on %s in %s", mapIndex.c_str(), dedicatedActor->cell.getDescription().c_str());
                    Cell *newCell = cellController->getCell(dedicatedActor->cell);
                    LocalActor *localActor = new LocalActor();
                    localActor->cell = dedicatedActor->cell;
                    localActor->setPtr(dedicatedActor->getPtr());
                    localActor->position = dedicatedActor->position;
                    localActor->direction = dedicatedActor->direction;
                    localActor->movementFlags = dedicatedActor->movementFlags;
                    localActor->drawState = dedicatedActor->drawState;
                    localActor->isFlying = dedicatedActor->isFlying;
                    localActor->creatureStats = dedicatedActor->creatureStats;

                    newCell->localActors[mapIndex] = localActor;
                    cellController->setLocalActorRecord(mapIndex, newCell->getDescription());
                }

                LOG_APPEND(Log::LOG_INFO, "- Deleting DedicatedActor %s which is no longer needed", mapIndex.c_str(), getDescription().c_str());
                cellController->removeDedicatedActorRecord(mapIndex);
                delete dedicatedActor;
            }

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
    for (const auto &mergedRef : store->getMergedRefs())
    {
        if (mergedRef->mClass->isActor())
        {
            MWWorld::Ptr ptr(mergedRef, store);

            // If this Ptr is lacking a unique index, ignore it
            if (ptr.getCellRef().getRefNum().mIndex == 0 && ptr.getCellRef().getMpNum() == 0) continue;

            std::string mapIndex = Main::get().getCellController()->generateMapIndex(ptr);

            // Only initialize this actor if it isn't already initialized
            if (localActors.count(mapIndex) == 0)
                initializeLocalActor(ptr);
        }
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
    for (const auto &baseActor : actorList.baseActors)
    {
        std::string mapIndex = Main::get().getCellController()->generateMapIndex(baseActor);

        // If this key doesn't exist, create it
        if (dedicatedActors.count(mapIndex) == 0)
        {
            MWWorld::Ptr ptrFound = store->searchExact(baseActor.refNumIndex, baseActor.mpNum);

            if (!ptrFound) return;

            initializeDedicatedActor(ptrFound);
        }
    }
}

void Cell::uninitializeLocalActors()
{
    for (const auto &actor : localActors)
    {
        Main::get().getCellController()->removeLocalActorRecord(actor.first);
        delete actor.second;
    }

    localActors.clear();
}

void Cell::uninitializeDedicatedActors()
{
    for (const auto &actor : dedicatedActors)
    {
        Main::get().getCellController()->removeDedicatedActorRecord(actor.first);
        delete actor.second;
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

bool Cell::hasLocalAuthority()
{
    return authorityGuid == Main::get().getLocalPlayer()->guid;
}

void Cell::setAuthority(const RakNet::RakNetGUID& guid)
{
    authorityGuid = guid;
}

MWWorld::CellStore *Cell::getCellStore()
{
    return store;
}

std::string Cell::getDescription()
{
    return store->getCell()->getDescription();
}
