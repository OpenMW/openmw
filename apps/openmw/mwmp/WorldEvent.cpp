#include "WorldEvent.hpp"
#include "Main.hpp"
#include "Networking.hpp"
#include "MechanicsHelper.hpp"
#include "LocalPlayer.hpp"
#include "DedicatedPlayer.hpp"
#include "PlayerList.hpp"
#include "CellController.hpp"

#include <components/openmw-mp/Log.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwmechanics/aifollow.hpp"
#include "../mwmechanics/spellcasting.hpp"
#include "../mwmechanics/summoning.hpp"

#include "../mwrender/animation.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/manualref.hpp"

using namespace mwmp;
using namespace std;

WorldEvent::WorldEvent()
{

}

WorldEvent::~WorldEvent()
{

}

Networking *WorldEvent::getNetworking()
{
    return mwmp::Main::get().getNetworking();
}

void WorldEvent::reset()
{
    cell.blank();
    worldObjects.clear();
    guid = mwmp::Main::get().getNetworking()->getLocalPlayer()->guid;
}

void WorldEvent::addObject(WorldObject worldObject)
{
    worldObjects.push_back(worldObject);
}

void WorldEvent::editContainers(MWWorld::CellStore* cellStore)
{
    WorldObject worldObject;

    for (unsigned int i = 0; i < worldObjectCount; i++)
    {
        worldObject = worldObjects.at(i);

        //LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s, %i, %i", worldObject.refId.c_str(), worldObject.refNumIndex, worldObject.mpNum);

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refNumIndex, worldObject.mpNum);

        if (ptrFound)
        {
            //LOG_APPEND(Log::LOG_VERBOSE, "-- Found %s, %i, %i", ptrFound.getCellRef().getRefId().c_str(),
            //                   ptrFound.getCellRef().getRefNum(), ptrFound.getCellRef().getMpNum());

            MWWorld::ContainerStore& containerStore = ptrFound.getClass().getContainerStore(ptrFound);

            // If we are setting the entire contents, clear the current ones
            if (action == BaseEvent::SET)
                containerStore.clear();

            MWWorld::Ptr ownerPtr = MWBase::Environment::get().getWorld()->getPlayerPtr();
            for (const auto &containerItem : worldObject.containerItems)
            {
                if (action == BaseEvent::ADD || action == BaseEvent::SET)
                {
                    // Create a ManualRef to be able to set item charge
                    MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(), containerItem.refId, 1);
                    MWWorld::Ptr newPtr = ref.getPtr();

                    if (containerItem.count > 1)
                        newPtr.getRefData().setCount(containerItem.count);

                    if (containerItem.charge > -1)
                        newPtr.getCellRef().setCharge(containerItem.charge);

                    containerStore.add(newPtr, containerItem.count, ownerPtr, true);
                }
                else if (action == BaseEvent::REMOVE)
                {
                    // We have to find the right item ourselves because ContainerStore has no method
                    // accounting for charge
                    for (const auto ptr : containerStore)
                    {
                        if (Misc::StringUtils::ciEqual(ptr.getCellRef().getRefId(), containerItem.refId))
                        {
                            if (ptr.getCellRef().getCharge() == containerItem.charge &&
                                    ptr.getRefData().getCount() == containerItem.count)
                            {
                                // Is this an actor's container? If so, unequip this item if it was equipped
                                if (ptrFound.getClass().isActor())
                                {
                                    MWWorld::InventoryStore& invStore = ptrFound.getClass().getInventoryStore(ptrFound);

                                    if (invStore.isEquipped(ptr))
                                        invStore.unequipItemQuantity(ptr, ptrFound, containerItem.count);
                                }

                                containerStore.remove(ptr, containerItem.actionCount, ownerPtr);
                            }
                        }
                    }
                }
            }

            // Was this a SET or ADD action on an actor's container, and are we the authority
            // over the actor? If so, autoequip the actor
            if ((action == BaseEvent::ADD || action == BaseEvent::SET) && ptrFound.getClass().isActor() &&
                mwmp::Main::get().getCellController()->isLocalActor(ptrFound))
            {
                MWWorld::InventoryStore& invStore = ptrFound.getClass().getInventoryStore(ptrFound);
                invStore.autoEquip(ptrFound);
            }

            // If we are in a container, and it happens to be this container, update its view
            if (MWBase::Environment::get().getWindowManager()->containsMode(MWGui::GM_Container))
            {
                CurrentContainer *currentContainer = &mwmp::Main::get().getLocalPlayer()->currentContainer;

                if (currentContainer->refNumIndex == ptrFound.getCellRef().getRefNum().mIndex &&
                    currentContainer->mpNum == ptrFound.getCellRef().getMpNum())
                {
                    MWBase::Environment::get().getWindowManager()->removeGuiMode(MWGui::GM_Container);
                    MWBase::Environment::get().getWindowManager()->openContainer(ptrFound, currentContainer->loot);
                }
            }
        }
    }
}

void WorldEvent::placeObjects(MWWorld::CellStore* cellStore)
{
    for (const auto &worldObject : worldObjects)
    {
        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s, %i, %i, charge: %i, count: %i", worldObject.refId.c_str(),
                   worldObject.refNumIndex, worldObject.mpNum, worldObject.charge, worldObject.count);

        MWWorld::Ptr ptrFound = cellStore->searchExact(0, worldObject.mpNum);

        // Only create this object if it doesn't already exist
        if (!ptrFound)
        {
            MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(), worldObject.refId, 1);
            MWWorld::Ptr newPtr = ref.getPtr();

            if (worldObject.charge > -1)
                newPtr.getCellRef().setCharge(worldObject.charge);

            if (worldObject.count > 1)
                newPtr.getRefData().setCount(worldObject.count);

            newPtr.getCellRef().setGoldValue(worldObject.goldValue);
            newPtr = MWBase::Environment::get().getWorld()->placeObject(newPtr, cellStore, worldObject.position);

            // Because gold automatically gets replaced with a new object, make sure we set the mpNum at the end
            newPtr.getCellRef().setMpNum(worldObject.mpNum);
        }
        else
            LOG_APPEND(Log::LOG_VERBOSE, "-- Object already existed!");
    }
}

void WorldEvent::spawnObjects(MWWorld::CellStore* cellStore)
{
    for (const auto &worldObject : worldObjects)
    {
        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s, %i, %i", worldObject.refId.c_str(),
            worldObject.refNumIndex, worldObject.mpNum);

        MWWorld::Ptr ptrFound = cellStore->searchExact(0, worldObject.mpNum);

        // Only create this object if it doesn't already exist
        if (!ptrFound)
        {
            MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(), worldObject.refId, 1);
            MWWorld::Ptr newPtr = ref.getPtr();

            newPtr.getCellRef().setMpNum(worldObject.mpNum);

            newPtr = MWBase::Environment::get().getWorld()->placeObject(newPtr, cellStore, worldObject.position);

            if (worldObject.hasMaster)
            {
                MWWorld::Ptr masterPtr;

                if (worldObject.master.refId.empty())
                    masterPtr = MechanicsHelper::getPlayerPtr(worldObject.master);
                else
                    masterPtr = cellStore->searchExact(worldObject.master.refNumIndex, worldObject.master.mpNum);

                if (masterPtr)
                {
                    LOG_APPEND(Log::LOG_VERBOSE, "-- Actor has master: %s", masterPtr.getCellRef().getRefId().c_str());

                    MWMechanics::AiFollow package(masterPtr.getCellRef().getRefId());
                    newPtr.getClass().getCreatureStats(newPtr).getAiSequence().stack(package, newPtr);

                    MWRender::Animation* anim = MWBase::Environment::get().getWorld()->getAnimation(newPtr);
                    if (anim)
                    {
                        const ESM::Static* fx = MWBase::Environment::get().getWorld()->getStore().get<ESM::Static>()
                            .search("VFX_Summon_Start");
                        if (fx)
                            anim->addEffect("meshes\\" + fx->mModel, -1, false);
                    }

                    int creatureActorId = newPtr.getClass().getCreatureStats(newPtr).getActorId();

                    MWMechanics::CreatureStats& masterCreatureStats = masterPtr.getClass().getCreatureStats(masterPtr);
                    masterCreatureStats.setSummonedCreatureActorId(worldObject.refId, creatureActorId);
                }
            }
        }
        else
            LOG_APPEND(Log::LOG_VERBOSE, "-- Actor already existed!");
    }
}

void WorldEvent::deleteObjects(MWWorld::CellStore* cellStore)
{
    for (const auto &worldObject : worldObjects)
    {
        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s, %i, %i", worldObject.refId.c_str(), worldObject.refNumIndex, worldObject.mpNum);

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refNumIndex, worldObject.mpNum);

        if (ptrFound)
        {
            LOG_APPEND(Log::LOG_VERBOSE, "-- Found %s, %i, %i", ptrFound.getCellRef().getRefId().c_str(),
                               ptrFound.getCellRef().getRefNum(), ptrFound.getCellRef().getMpNum());

            // If we are in a container, and it happens to be this object, exit it
            if (MWBase::Environment::get().getWindowManager()->containsMode(MWGui::GM_Container))
            {
                CurrentContainer *currentContainer = &mwmp::Main::get().getLocalPlayer()->currentContainer;

                if (currentContainer->refNumIndex == ptrFound.getCellRef().getRefNum().mIndex &&
                    currentContainer->mpNum == ptrFound.getCellRef().getMpNum())
                {
                    MWBase::Environment::get().getWindowManager()->removeGuiMode(MWGui::GM_Container);
                    MWBase::Environment::get().getWindowManager()->setDragDrop(false);
                }
            }

            MWBase::Environment::get().getWorld()->deleteObject(ptrFound);
        }
    }
}

void WorldEvent::lockObjects(MWWorld::CellStore* cellStore)
{
    for (const auto &worldObject : worldObjects)
    {
        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s, %i, %i", worldObject.refId.c_str(), worldObject.refNumIndex, worldObject.mpNum);

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refNumIndex, worldObject.mpNum);

        if (ptrFound)
        {
            LOG_APPEND(Log::LOG_VERBOSE, "-- Found %s, %i, %i", ptrFound.getCellRef().getRefId().c_str(),
                               ptrFound.getCellRef().getRefNum(), ptrFound.getCellRef().getMpNum());

            if (worldObject.lockLevel > 0)
                ptrFound.getClass().lock(ptrFound, worldObject.lockLevel);
            else
                ptrFound.getClass().unlock(ptrFound);
        }
    }
}

void WorldEvent::triggerTrapObjects(MWWorld::CellStore* cellStore)
{
    for (const auto &worldObject : worldObjects)
    {
        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s, %i, %i", worldObject.refId.c_str(), worldObject.refNumIndex, worldObject.mpNum);

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refNumIndex, worldObject.mpNum);

        if (ptrFound)
        {
            LOG_APPEND(Log::LOG_VERBOSE, "-- Found %s, %i, %i", ptrFound.getCellRef().getRefId().c_str(),
                ptrFound.getCellRef().getRefNum(), ptrFound.getCellRef().getMpNum());

            if (!worldObject.isDisarmed)
            {
                MWMechanics::CastSpell cast(ptrFound, ptrFound);
                cast.mHitPosition = worldObject.position.asVec3();
                cast.cast(ptrFound.getCellRef().getTrap());
            }

            ptrFound.getCellRef().setTrap("");
        }
    }
}

void WorldEvent::scaleObjects(MWWorld::CellStore* cellStore)
{
    for (const auto &worldObject : worldObjects)
    {
        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s, %i, %i, scale: %f", worldObject.refId.c_str(), worldObject.refNumIndex,
            worldObject.mpNum, worldObject.scale);

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refNumIndex, worldObject.mpNum);

        if (ptrFound)
        {
            LOG_APPEND(Log::LOG_VERBOSE, "-- Found %s, %i, %i", ptrFound.getCellRef().getRefId().c_str(),
                               ptrFound.getCellRef().getRefNum(), ptrFound.getCellRef().getMpNum());

            MWBase::Environment::get().getWorld()->scaleObject(ptrFound, worldObject.scale);
        }
    }
}

void WorldEvent::setObjectStates(MWWorld::CellStore* cellStore)
{
    for (const auto &worldObject : worldObjects)
    {
        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s, %i, %i, state: %s", worldObject.refId.c_str(), worldObject.refNumIndex,
            worldObject.mpNum, worldObject.objectState ? "true" : "false");

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refNumIndex, worldObject.mpNum);

        if (ptrFound)
        {
            LOG_APPEND(Log::LOG_VERBOSE, "-- Found %s, %i, %i", ptrFound.getCellRef().getRefId().c_str(),
                ptrFound.getCellRef().getRefNum(), ptrFound.getCellRef().getMpNum());

            if (worldObject.objectState)
                MWBase::Environment::get().getWorld()->enable(ptrFound);
            else
                MWBase::Environment::get().getWorld()->disable(ptrFound);
        }
    }
}

void WorldEvent::moveObjects(MWWorld::CellStore* cellStore)
{
    for (const auto &worldObject : worldObjects)
    {
        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s, %i, %i", worldObject.refId.c_str(), worldObject.refNumIndex, worldObject.mpNum);

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refNumIndex, worldObject.mpNum);

        if (ptrFound)
        {
            LOG_APPEND(Log::LOG_VERBOSE, "-- Found %s, %i, %i", ptrFound.getCellRef().getRefId().c_str(),
                               ptrFound.getCellRef().getRefNum(), ptrFound.getCellRef().getMpNum());

            MWBase::Environment::get().getWorld()->moveObject(ptrFound, worldObject.position.pos[0], worldObject.position.pos[1],
                                                              worldObject.position.pos[2]);
        }
    }
}

void WorldEvent::rotateObjects(MWWorld::CellStore* cellStore)
{
    for (const auto &worldObject : worldObjects)
    {
        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s, %i, %i", worldObject.refId.c_str(), worldObject.refNumIndex, worldObject.mpNum);

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refNumIndex, worldObject.mpNum);

        if (ptrFound)
        {
            LOG_APPEND(Log::LOG_VERBOSE, "-- Found %s, %i, %i", ptrFound.getCellRef().getRefId().c_str(),
                               ptrFound.getCellRef().getRefNum(), ptrFound.getCellRef().getMpNum());

            MWBase::Environment::get().getWorld()->rotateObject(ptrFound,
                worldObject.position.rot[0], worldObject.position.rot[1], worldObject.position.rot[2]);
        }
    }
}

void WorldEvent::animateObjects(MWWorld::CellStore* cellStore)
{
    for (const auto &worldObject : worldObjects)
    {
        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s, %i, %i", worldObject.refId.c_str(), worldObject.refNumIndex, worldObject.mpNum);

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refNumIndex, worldObject.mpNum);

        if (ptrFound)
        {
            LOG_APPEND(Log::LOG_VERBOSE, "-- Found %s, %i, %i", ptrFound.getCellRef().getRefId().c_str(),
                               ptrFound.getCellRef().getRefNum(), ptrFound.getCellRef().getMpNum());

            MWBase::MechanicsManager * mechanicsManager = MWBase::Environment::get().getMechanicsManager();
            mechanicsManager->playAnimationGroup(ptrFound, worldObject.animGroup, worldObject.animMode,
                                                 std::numeric_limits<int>::max(), true);
        }
    }
}

void WorldEvent::activateDoors(MWWorld::CellStore* cellStore)
{
    for (const auto &worldObject : worldObjects)
    {
        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s, %i, %i", worldObject.refId.c_str(), worldObject.refNumIndex, worldObject.mpNum);

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refNumIndex, worldObject.mpNum);

        if (ptrFound)
        {
            LOG_APPEND(Log::LOG_VERBOSE, "-- Found %s, %i, %i", ptrFound.getCellRef().getRefId().c_str(),
                               ptrFound.getCellRef().getRefNum(), ptrFound.getCellRef().getMpNum());

            ptrFound.getClass().setDoorState(ptrFound, worldObject.doorState);
            MWBase::Environment::get().getWorld()->saveDoorState(ptrFound, worldObject.doorState);
        }
    }
}

void WorldEvent::setLocalShorts(MWWorld::CellStore* cellStore)
{
    for (const auto &worldObject : worldObjects)
    {
        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s, %i, %i, index: %i, shortVal: %i", worldObject.refId.c_str(),
                   worldObject.refNumIndex, worldObject.mpNum, worldObject.index, worldObject.shortVal);

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refNumIndex, worldObject.mpNum);

        if (ptrFound)
        {
            LOG_APPEND(Log::LOG_VERBOSE, "-- Found %s, %i, %i", ptrFound.getCellRef().getRefId().c_str(),
                               ptrFound.getCellRef().getRefNum(), ptrFound.getCellRef().getMpNum());

            ptrFound.getRefData().getLocals().mShorts.at(worldObject.index) = worldObject.shortVal;
        }
    }
}

void WorldEvent::setLocalFloats(MWWorld::CellStore* cellStore)
{
    for (const auto &worldObject : worldObjects)
    {
        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s, %i, %i, index: %i, floatVal: %f", worldObject.refId.c_str(),
                   worldObject.refNumIndex, worldObject.mpNum, worldObject.index, worldObject.floatVal);

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refNumIndex, worldObject.mpNum);

        if (ptrFound)
        {
            LOG_APPEND(Log::LOG_VERBOSE, "-- Found %s, %i, %i", ptrFound.getCellRef().getRefId().c_str(),
                               ptrFound.getCellRef().getRefNum(), ptrFound.getCellRef().getMpNum());

            ptrFound.getRefData().getLocals().mFloats.at(worldObject.index) = worldObject.floatVal;
        }
    }
}

void WorldEvent::setMemberShorts()
{
    for (const auto &worldObject : worldObjects)
    {
        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s, index: %i, shortVal: %i", worldObject.refId.c_str(),
                   worldObject.index, worldObject.shortVal);

        // Mimic the way a Ptr is fetched in InterpreterContext for similar situations
        MWWorld::Ptr ptrFound = MWBase::Environment::get().getWorld()->searchPtr(worldObject.refId, false);

        if (!ptrFound.isEmpty())
        {
            LOG_APPEND(Log::LOG_VERBOSE, "-- Found %s, %i, %i", ptrFound.getCellRef().getRefId().c_str(),
                               ptrFound.getCellRef().getRefNum(), ptrFound.getCellRef().getMpNum());

            std::string scriptId = ptrFound.getClass().getScript(ptrFound);

            ptrFound.getRefData().setLocals(
                *MWBase::Environment::get().getWorld()->getStore().get<ESM::Script>().find(scriptId));

            ptrFound.getRefData().getLocals().mShorts.at(worldObject.index) = worldObject.shortVal;;
        }
    }
}

void WorldEvent::setGlobalShorts()
{
    for (const auto &worldObject : worldObjects)
    {
        LOG_APPEND(Log::LOG_VERBOSE, "- varName: %s, shortVal: %i", worldObject.varName.c_str(), worldObject.shortVal);

        MWBase::Environment::get().getWorld()->setGlobalInt(worldObject.varName, worldObject.shortVal);
    }
}

void WorldEvent::playMusic()
{
    for (const auto &worldObject : worldObjects)
    {
        LOG_APPEND(Log::LOG_VERBOSE, "- filename: %s", worldObject.filename.c_str());

        MWBase::Environment::get().getSoundManager()->streamMusic(worldObject.filename);
    }
}

void WorldEvent::playVideo()
{
    for (const auto &worldObject : worldObjects)
    {
        LOG_APPEND(Log::LOG_VERBOSE, "- filename: %s, allowSkipping: %s", worldObject.filename.c_str(),
            worldObject.allowSkipping ? "true" : "false");

        MWBase::Environment::get().getWindowManager()->playVideo(worldObject.filename, worldObject.allowSkipping);
    }
}

void WorldEvent::addObjectPlace(const MWWorld::Ptr& ptr)
{
    cell = *ptr.getCell()->getCell();

    mwmp::WorldObject worldObject;
    worldObject.refId = ptr.getCellRef().getRefId();
    worldObject.refNumIndex = ptr.getCellRef().getRefNum().mIndex;
    worldObject.mpNum = 0;
    worldObject.charge = ptr.getCellRef().getCharge();

    // Make sure we send the RefData position instead of the CellRef one, because that's what
    // we actually see on this client
    worldObject.position = ptr.getRefData().getPosition();

    // We have to get the count from the dropped object because it gets changed
    // automatically for stacks of gold
    worldObject.count = ptr.getRefData().getCount();

    // Get the real count of gold in a stack
    worldObject.goldValue = ptr.getCellRef().getGoldValue();

    addObject(worldObject);
}

void WorldEvent::addObjectSpawn(const MWWorld::Ptr& ptr)
{
    cell = *ptr.getCell()->getCell();

    mwmp::WorldObject worldObject;
    worldObject.refId = ptr.getCellRef().getRefId();
    worldObject.refNumIndex = ptr.getCellRef().getRefNum().mIndex;
    worldObject.mpNum = 0;
    worldObject.hasMaster = false;

    // Make sure we send the RefData position instead of the CellRef one, because that's what
    // we actually see on this client
    worldObject.position = ptr.getRefData().getPosition();

    addObject(worldObject);
}

void WorldEvent::addObjectSpawn(const MWWorld::Ptr& ptr, const MWWorld::Ptr& master)
{
    cell = *ptr.getCell()->getCell();

    mwmp::WorldObject worldObject;
    worldObject.refId = ptr.getCellRef().getRefId();
    worldObject.refNumIndex = ptr.getCellRef().getRefNum().mIndex;
    worldObject.mpNum = 0;

    worldObject.hasMaster = true;

    if (master == MWBase::Environment::get().getWorld()->getPlayerPtr())
    {
        worldObject.master.guid = mwmp::Main::get().getLocalPlayer()->guid;
        worldObject.master.refId.clear();
    }
    else if (mwmp::PlayerList::isDedicatedPlayer(master))
    {
        worldObject.master.guid = mwmp::PlayerList::getPlayer(master)->guid;
        worldObject.master.refId.clear();
    }
    else
    {
        MWWorld::CellRef *masterRef = &master.getCellRef();

        worldObject.master.refId = masterRef->getRefId();
        worldObject.master.refNumIndex = masterRef->getRefNum().mIndex;
        worldObject.master.mpNum = masterRef->getMpNum();
    }

    // Make sure we send the RefData position instead of the CellRef one, because that's what
    // we actually see on this client
    worldObject.position = ptr.getRefData().getPosition();

    addObject(worldObject);
}

void WorldEvent::addObjectDelete(const MWWorld::Ptr& ptr)
{
    cell = *ptr.getCell()->getCell();

    mwmp::WorldObject worldObject;
    worldObject.refId = ptr.getCellRef().getRefId();
    worldObject.refNumIndex = ptr.getCellRef().getRefNum().mIndex;
    worldObject.mpNum = ptr.getCellRef().getMpNum();
    addObject(worldObject);
}

void WorldEvent::addObjectLock(const MWWorld::Ptr& ptr, int lockLevel)
{
    cell = *ptr.getCell()->getCell();

    mwmp::WorldObject worldObject;
    worldObject.refId = ptr.getCellRef().getRefId();
    worldObject.refNumIndex = ptr.getCellRef().getRefNum().mIndex;
    worldObject.mpNum = ptr.getCellRef().getMpNum();
    worldObject.lockLevel = lockLevel;
    addObject(worldObject);
}

void WorldEvent::addObjectTrap(const MWWorld::Ptr& ptr, const ESM::Position& pos, bool isDisarmed)
{
    cell = *ptr.getCell()->getCell();

    mwmp::WorldObject worldObject;
    worldObject.refId = ptr.getCellRef().getRefId();
    worldObject.refNumIndex = ptr.getCellRef().getRefNum().mIndex;
    worldObject.mpNum = ptr.getCellRef().getMpNum();
    worldObject.isDisarmed = isDisarmed;
    worldObject.position = pos;
    addObject(worldObject);
}

void WorldEvent::addObjectScale(const MWWorld::Ptr& ptr, float scale)
{
    cell = *ptr.getCell()->getCell();

    mwmp::WorldObject worldObject;
    worldObject.refId = ptr.getCellRef().getRefId();
    worldObject.refNumIndex = ptr.getCellRef().getRefNum().mIndex;
    worldObject.mpNum = ptr.getCellRef().getMpNum();
    worldObject.scale = scale;
    addObject(worldObject);
}

void WorldEvent::addObjectState(const MWWorld::Ptr& ptr, bool objectState)
{
    cell = *ptr.getCell()->getCell();

    mwmp::WorldObject worldObject;
    worldObject.refId = ptr.getCellRef().getRefId();
    worldObject.refNumIndex = ptr.getCellRef().getRefNum().mIndex;
    worldObject.mpNum = ptr.getCellRef().getMpNum();
    worldObject.objectState = objectState;
    addObject(worldObject);
}

void WorldEvent::addObjectAnimPlay(const MWWorld::Ptr& ptr, std::string group, int mode)
{
    cell = *ptr.getCell()->getCell();

    mwmp::WorldObject worldObject;
    worldObject.refId = ptr.getCellRef().getRefId();
    worldObject.refNumIndex = ptr.getCellRef().getRefNum().mIndex;
    worldObject.mpNum = ptr.getCellRef().getMpNum();
    worldObject.animGroup = group;
    worldObject.animMode = mode;
    addObject(worldObject);
}

void WorldEvent::addDoorState(const MWWorld::Ptr& ptr, int state)
{
    cell = *ptr.getCell()->getCell();

    mwmp::WorldObject worldObject;
    worldObject.refId = ptr.getCellRef().getRefId();
    worldObject.refNumIndex = ptr.getCellRef().getRefNum().mIndex;
    worldObject.mpNum = ptr.getCellRef().getMpNum();
    worldObject.doorState = state;
    addObject(worldObject);
}

void WorldEvent::addMusicPlay(std::string filename)
{
    mwmp::WorldObject worldObject;
    worldObject.filename = filename;
    addObject(worldObject);
}

void WorldEvent::addVideoPlay(std::string filename, bool allowSkipping)
{
    mwmp::WorldObject worldObject;
    worldObject.filename = filename;
    worldObject.allowSkipping = allowSkipping;
    addObject(worldObject);
}

void WorldEvent::addScriptLocalShort(const MWWorld::Ptr& ptr, int index, int shortVal)
{
    cell = *ptr.getCell()->getCell();

    mwmp::WorldObject worldObject;
    worldObject.refId = ptr.getCellRef().getRefId();
    worldObject.refNumIndex = ptr.getCellRef().getRefNum().mIndex;
    worldObject.mpNum = ptr.getCellRef().getMpNum();
    worldObject.index = index;
    worldObject.shortVal = shortVal;
    addObject(worldObject);
}

void WorldEvent::addScriptLocalFloat(const MWWorld::Ptr& ptr, int index, float floatVal)
{
    cell = *ptr.getCell()->getCell();

    mwmp::WorldObject worldObject;
    worldObject.refId = ptr.getCellRef().getRefId();
    worldObject.refNumIndex = ptr.getCellRef().getRefNum().mIndex;
    worldObject.mpNum = ptr.getCellRef().getMpNum();
    worldObject.index = index;
    worldObject.floatVal = floatVal;
    addObject(worldObject);
}

void WorldEvent::addScriptMemberShort(std::string refId, int index, int shortVal)
{
    mwmp::WorldObject worldObject;
    worldObject.refId = refId;
    worldObject.index = index;
    worldObject.shortVal = shortVal;
    addObject(worldObject);
}

void WorldEvent::addScriptGlobalShort(std::string varName, int shortVal)
{
    mwmp::WorldObject worldObject;
    worldObject.varName = varName;
    worldObject.shortVal = shortVal;
    addObject(worldObject);
}

void WorldEvent::sendObjectPlace()
{
    LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Sending ID_OBJECT_PLACE about %s", cell.getDescription().c_str());

    for (const auto &worldObject : worldObjects)
        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s, count: %i", worldObject.refId.c_str(), worldObject.count);

    mwmp::Main::get().getNetworking()->getWorldPacket(ID_OBJECT_PLACE)->setEvent(this);
    mwmp::Main::get().getNetworking()->getWorldPacket(ID_OBJECT_PLACE)->Send();
}

void WorldEvent::sendObjectSpawn()
{
    LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Sending ID_OBJECT_SPAWN about %s", cell.getDescription().c_str());

    for (const auto &worldObject : worldObjects)
        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s-%i", worldObject.refId.c_str(), worldObject.refNumIndex);

    mwmp::Main::get().getNetworking()->getWorldPacket(ID_OBJECT_SPAWN)->setEvent(this);
    mwmp::Main::get().getNetworking()->getWorldPacket(ID_OBJECT_SPAWN)->Send();
}

void WorldEvent::sendObjectDelete()
{
    mwmp::Main::get().getNetworking()->getWorldPacket(ID_OBJECT_DELETE)->setEvent(this);
    mwmp::Main::get().getNetworking()->getWorldPacket(ID_OBJECT_DELETE)->Send();
}

void WorldEvent::sendObjectLock()
{
    mwmp::Main::get().getNetworking()->getWorldPacket(ID_OBJECT_LOCK)->setEvent(this);
    mwmp::Main::get().getNetworking()->getWorldPacket(ID_OBJECT_LOCK)->Send();
}

void WorldEvent::sendObjectTrap()
{
    mwmp::Main::get().getNetworking()->getWorldPacket(ID_OBJECT_TRAP)->setEvent(this);
    mwmp::Main::get().getNetworking()->getWorldPacket(ID_OBJECT_TRAP)->Send();
}

void WorldEvent::sendObjectScale()
{
    mwmp::Main::get().getNetworking()->getWorldPacket(ID_OBJECT_SCALE)->setEvent(this);
    mwmp::Main::get().getNetworking()->getWorldPacket(ID_OBJECT_SCALE)->Send();
}

void WorldEvent::sendObjectState()
{
    mwmp::Main::get().getNetworking()->getWorldPacket(ID_OBJECT_STATE)->setEvent(this);
    mwmp::Main::get().getNetworking()->getWorldPacket(ID_OBJECT_STATE)->Send();
}

void WorldEvent::sendObjectAnimPlay()
{
    mwmp::Main::get().getNetworking()->getWorldPacket(ID_OBJECT_ANIM_PLAY)->setEvent(this);
    mwmp::Main::get().getNetworking()->getWorldPacket(ID_OBJECT_ANIM_PLAY)->Send();
}

void WorldEvent::sendDoorState()
{
    LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Sending ID_DOOR_STATE about %s", cell.getDescription().c_str());

    for (const auto &worldObject : worldObjects)
        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s-%i, state: %s", worldObject.refId.c_str(), worldObject.refNumIndex,
                   worldObject.doorState ? "true" : "false");

    mwmp::Main::get().getNetworking()->getWorldPacket(ID_DOOR_STATE)->setEvent(this);
    mwmp::Main::get().getNetworking()->getWorldPacket(ID_DOOR_STATE)->Send();
}

void WorldEvent::sendMusicPlay()
{
    mwmp::Main::get().getNetworking()->getWorldPacket(ID_MUSIC_PLAY)->setEvent(this);
    mwmp::Main::get().getNetworking()->getWorldPacket(ID_MUSIC_PLAY)->Send();
}

void WorldEvent::sendVideoPlay()
{
    mwmp::Main::get().getNetworking()->getWorldPacket(ID_VIDEO_PLAY)->setEvent(this);
    mwmp::Main::get().getNetworking()->getWorldPacket(ID_VIDEO_PLAY)->Send();
}

void WorldEvent::sendScriptLocalShort()
{
    LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Sending ID_SCRIPT_LOCAL_SHORT about %s", cell.getDescription().c_str());

    for (const auto &worldObject : worldObjects)
        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s-%i, index: %i, shortVal: %i", worldObject.refId.c_str(),
                   worldObject.refNumIndex, worldObject.index, worldObject.shortVal);

    mwmp::Main::get().getNetworking()->getWorldPacket(ID_SCRIPT_LOCAL_SHORT)->setEvent(this);
    mwmp::Main::get().getNetworking()->getWorldPacket(ID_SCRIPT_LOCAL_SHORT)->Send();
}

void WorldEvent::sendScriptLocalFloat()
{
    LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Sending ID_SCRIPT_LOCAL_FLOAT about %s", cell.getDescription().c_str());

    for (const auto &worldObject : worldObjects)
        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s-%i, index: %i, floatVal: %f", worldObject.refId.c_str(), 
                   worldObject.refNumIndex, worldObject.index, worldObject.floatVal);

    mwmp::Main::get().getNetworking()->getWorldPacket(ID_SCRIPT_LOCAL_FLOAT)->setEvent(this);
    mwmp::Main::get().getNetworking()->getWorldPacket(ID_SCRIPT_LOCAL_FLOAT)->Send();
}

void WorldEvent::sendScriptMemberShort()
{
    LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Sending ID_SCRIPT_MEMBER_SHORT");

    for (const auto &worldObject : worldObjects)
        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s, index: %i, shortVal: %i", worldObject.refId.c_str(),
                   worldObject.index, worldObject.shortVal);

    mwmp::Main::get().getNetworking()->getWorldPacket(ID_SCRIPT_MEMBER_SHORT)->setEvent(this);
    mwmp::Main::get().getNetworking()->getWorldPacket(ID_SCRIPT_MEMBER_SHORT)->Send();
}

void WorldEvent::sendScriptGlobalShort()
{
    LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Sending ID_SCRIPT_GLOBAL_SHORT");

    for (const auto &worldObject : worldObjects)
        LOG_APPEND(Log::LOG_VERBOSE, "- varName: %s, shortVal: %i", worldObject.varName.c_str(), worldObject.shortVal);

    mwmp::Main::get().getNetworking()->getWorldPacket(ID_SCRIPT_GLOBAL_SHORT)->setEvent(this);
    mwmp::Main::get().getNetworking()->getWorldPacket(ID_SCRIPT_GLOBAL_SHORT)->Send();
}

void WorldEvent::sendContainers(MWWorld::CellStore* cellStore)
{
    reset();
    cell = *cellStore->getCell();
    action = BaseEvent::SET;

    MWWorld::CellRefList<ESM::Container> *containerList = cellStore->getContainers();

    for (auto &container : containerList->mList)
    {
        mwmp::WorldObject worldObject;
        worldObject.refId = container.mRef.getRefId();
        worldObject.refNumIndex = container.mRef.getRefNum().mIndex;
        worldObject.mpNum = container.mRef.getMpNum();

        MWWorld::ContainerStore& containerStore = container.mClass->getContainerStore(MWWorld::Ptr(&container, 0));

        for (const auto itemPtr : containerStore)
        {
            mwmp::ContainerItem containerItem;
            containerItem.refId = itemPtr.getCellRef().getRefId();
            containerItem.count = itemPtr.getRefData().getCount();
            containerItem.charge = itemPtr.getCellRef().getCharge();

            worldObject.containerItems.push_back(containerItem);
        }

        addObject(worldObject);
    }

    mwmp::Main::get().getNetworking()->getWorldPacket(ID_CONTAINER)->setEvent(this);
    mwmp::Main::get().getNetworking()->getWorldPacket(ID_CONTAINER)->Send();
}
