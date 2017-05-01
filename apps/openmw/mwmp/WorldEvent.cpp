#include "WorldEvent.hpp"
#include "Main.hpp"
#include "Networking.hpp"
#include "LocalPlayer.hpp"
#include "DedicatedPlayer.hpp"

#include <components/openmw-mp/Log.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/esmstore.hpp"
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
    objectChanges.objects.clear();
    guid = mwmp::Main::get().getNetworking()->getLocalPlayer()->guid;
}

void WorldEvent::addObject(WorldObject worldObject)
{
    objectChanges.objects.push_back(worldObject);
}

void WorldEvent::editContainers(MWWorld::CellStore* cellStore)
{
    WorldObject worldObject;

    for (unsigned int i = 0; i < objectChanges.count; i++)
    {
        worldObject = objectChanges.objects.at(i);

        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s, %i, %i", worldObject.refId.c_str(), worldObject.refNumIndex, worldObject.mpNum);

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refId, worldObject.refNumIndex, worldObject.mpNum);

        if (ptrFound)
        {
            LOG_APPEND(Log::LOG_VERBOSE, "-- Found %s, %i, %i", ptrFound.getCellRef().getRefId().c_str(),
                               ptrFound.getCellRef().getRefNum(), ptrFound.getCellRef().getMpNum());

            MWWorld::ContainerStore& containerStore = ptrFound.getClass().getContainerStore(ptrFound);

            // If we are setting the entire contents, clear the current ones
            if (action == BaseEvent::SET)
                containerStore.clear();

            for (unsigned int i = 0; i < worldObject.containerChanges.count; i++)
            {
                ContainerItem containerItem = worldObject.containerChanges.items.at(i);
                MWWorld::Ptr ownerPtr = MWBase::Environment::get().getWorld()->getPlayerPtr();

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
                    for (MWWorld::ContainerStoreIterator iter(containerStore.begin()); iter != containerStore.end(); ++iter)
                    {
                        if (Misc::StringUtils::ciEqual(iter->getCellRef().getRefId(), containerItem.refId))
                        {
                            if (iter->getCellRef().getCharge() == containerItem.charge &&
                                iter->getRefData().getCount() == containerItem.count)
                            {
                                containerStore.remove(*iter, containerItem.actionCount, ownerPtr);
                            }
                        }
                    }
                }
            }

            // If we are in a container, and it happens to be this container, update its view
            if (MWBase::Environment::get().getWindowManager()->containsMode(MWGui::GM_Container))
            {
                CurrentContainer *currentContainer = &mwmp::Main::get().getLocalPlayer()->currentContainer;

                if (currentContainer->refNumIndex == ptrFound.getCellRef().getRefNum().mIndex &&
                    Misc::StringUtils::ciEqual(currentContainer->refId, ptrFound.getCellRef().getRefId()))
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
    WorldObject worldObject;

    for (unsigned int i = 0; i < objectChanges.count; i++)
    {
        worldObject = objectChanges.objects.at(i);

        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s, %i, %i\n- charge: %i\n- count: %i", worldObject.refId.c_str(),
                   worldObject.refNumIndex, worldObject.mpNum, worldObject.charge, worldObject.count);

        MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(), worldObject.refId, 1);
        MWWorld::Ptr newPtr = ref.getPtr();

        if (worldObject.charge > -1)
            newPtr.getCellRef().setCharge(worldObject.charge);

        if (worldObject.count > 1)
            newPtr.getRefData().setCount(worldObject.count);

        newPtr.getCellRef().setMpNum(worldObject.mpNum);

        newPtr.getCellRef().setGoldValue(worldObject.goldValue);
        newPtr = MWBase::Environment::get().getWorld()->placeObject(newPtr, cellStore, worldObject.position);
    }
}

void WorldEvent::deleteObjects(MWWorld::CellStore* cellStore)
{
    WorldObject worldObject;

    for (unsigned int i = 0; i < objectChanges.count; i++)
    {
        worldObject = objectChanges.objects.at(i);

        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s, %i, %i", worldObject.refId.c_str(), worldObject.refNumIndex, worldObject.mpNum);

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refId, worldObject.refNumIndex, worldObject.mpNum);

        if (ptrFound)
        {
            LOG_APPEND(Log::LOG_VERBOSE, "-- Found %s, %i, %i", ptrFound.getCellRef().getRefId().c_str(),
                               ptrFound.getCellRef().getRefNum(), ptrFound.getCellRef().getMpNum());

            MWBase::Environment::get().getWorld()->deleteObject(ptrFound);
        }
    }
}

void WorldEvent::lockObjects(MWWorld::CellStore* cellStore)
{
    WorldObject worldObject;

    for (unsigned int i = 0; i < objectChanges.count; i++)
    {
        worldObject = objectChanges.objects.at(i);

        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s, %i, %i", worldObject.refId.c_str(), worldObject.refNumIndex, worldObject.mpNum);

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refId, worldObject.refNumIndex, worldObject.mpNum);

        if (ptrFound)
        {
            LOG_APPEND(Log::LOG_VERBOSE, "-- Found %s, %i, %i", ptrFound.getCellRef().getRefId().c_str(),
                               ptrFound.getCellRef().getRefNum(), ptrFound.getCellRef().getMpNum());

            ptrFound.getClass().lock(ptrFound, worldObject.lockLevel);
        }
    }
}

void WorldEvent::unlockObjects(MWWorld::CellStore* cellStore)
{
    WorldObject worldObject;

    for (unsigned int i = 0; i < objectChanges.count; i++)
    {
        worldObject = objectChanges.objects.at(i);

        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s, %i, %i", worldObject.refId.c_str(), worldObject.refNumIndex, worldObject.mpNum);

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refId, worldObject.refNumIndex, worldObject.mpNum);

        if (ptrFound)
        {
            LOG_APPEND(Log::LOG_VERBOSE, "-- Found %s, %i, %i", ptrFound.getCellRef().getRefId().c_str(),
                               ptrFound.getCellRef().getRefNum(), ptrFound.getCellRef().getMpNum());

            ptrFound.getClass().unlock(ptrFound);
        }
    }
}

void WorldEvent::scaleObjects(MWWorld::CellStore* cellStore)
{
    WorldObject worldObject;

    for (unsigned int i = 0; i < objectChanges.count; i++)
    {
        worldObject = objectChanges.objects.at(i);

        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s, %i, %i\n- scale: %f", worldObject.refId.c_str(), worldObject.refNumIndex,
            worldObject.mpNum, worldObject.scale);

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refId, worldObject.refNumIndex, worldObject.mpNum);

        if (ptrFound)
        {
            LOG_APPEND(Log::LOG_VERBOSE, "-- Found %s, %i, %i", ptrFound.getCellRef().getRefId().c_str(),
                               ptrFound.getCellRef().getRefNum(), ptrFound.getCellRef().getMpNum());

            MWBase::Environment::get().getWorld()->scaleObject(ptrFound, worldObject.scale);
        }
    }
}

void WorldEvent::moveObjects(MWWorld::CellStore* cellStore)
{
    WorldObject worldObject;

    for (unsigned int i = 0; i < objectChanges.count; i++)
    {
        worldObject = objectChanges.objects.at(i);

        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s, %i, %i", worldObject.refId.c_str(), worldObject.refNumIndex, worldObject.mpNum);

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refId, worldObject.refNumIndex, worldObject.mpNum);

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
    WorldObject worldObject;

    for (unsigned int i = 0; i < objectChanges.count; i++)
    {
        worldObject = objectChanges.objects.at(i);

        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s, %i, %i", worldObject.refId.c_str(), worldObject.refNumIndex, worldObject.mpNum);

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refId, worldObject.refNumIndex, worldObject.mpNum);

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
    WorldObject worldObject;

    for (unsigned int i = 0; i < objectChanges.count; i++)
    {
        worldObject = objectChanges.objects.at(i);

        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s, %i, %i", worldObject.refId.c_str(), worldObject.refNumIndex, worldObject.mpNum);

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refId, worldObject.refNumIndex, worldObject.mpNum);

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
    WorldObject worldObject;

    for (unsigned int i = 0; i < objectChanges.count; i++)
    {
        worldObject = objectChanges.objects.at(i);

        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s, %i, %i", worldObject.refId.c_str(), worldObject.refNumIndex, worldObject.mpNum);

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refId, worldObject.refNumIndex, worldObject.mpNum);

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
    WorldObject worldObject;

    for (unsigned int i = 0; i < objectChanges.count; i++)
    {
        worldObject = objectChanges.objects.at(i);

        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s, %i, %i\n- index: %i\n- shortVal: %i", worldObject.refId.c_str(),
                   worldObject.refNumIndex, worldObject.mpNum, worldObject.index, worldObject.shortVal);

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refId, worldObject.refNumIndex, worldObject.mpNum);

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
    WorldObject worldObject;

    for (unsigned int i = 0; i < objectChanges.count; i++)
    {
        worldObject = objectChanges.objects.at(i);

        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s, %i, %i\n- index: %i\n- floatVal: %f", worldObject.refId.c_str(),
                   worldObject.refNumIndex, worldObject.mpNum, worldObject.index, worldObject.floatVal);

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refId, worldObject.refNumIndex, worldObject.mpNum);

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
    WorldObject worldObject;

    for (unsigned int i = 0; i < objectChanges.count; i++)
    {
        worldObject = objectChanges.objects.at(i);

        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s\n- index: %i\n- shortVal: %i\n", worldObject.refId.c_str(),
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
    WorldObject worldObject;

    for (unsigned int i = 0; i < objectChanges.count; i++)
    {
        worldObject = objectChanges.objects.at(i);

        LOG_APPEND(Log::LOG_VERBOSE, "- varName: %s\n- shortVal: %i", worldObject.varName.c_str(), worldObject.shortVal);

        MWBase::Environment::get().getWorld()->setGlobalInt(worldObject.varName, worldObject.shortVal);
    }
}

void WorldEvent::playMusic()
{
    WorldObject worldObject;

    for (unsigned int i = 0; i < objectChanges.count; i++)
    {
        worldObject = objectChanges.objects.at(i);

        LOG_APPEND(Log::LOG_VERBOSE, "- filename: %s", worldObject.filename.c_str());

        MWBase::Environment::get().getSoundManager()->streamMusic(worldObject.filename);
    }
}

void WorldEvent::playVideo()
{
    WorldObject worldObject;

    for (unsigned int i = 0; i < objectChanges.count; i++)
    {
        worldObject = objectChanges.objects.at(i);

        LOG_APPEND(Log::LOG_VERBOSE, "- filename: %s\n- allowSkipping: %s", worldObject.filename.c_str(),
            worldObject.allowSkipping ? "true" : "false");

        MWBase::Environment::get().getWindowManager()->playVideo(worldObject.filename, worldObject.allowSkipping);
    }
}

void WorldEvent::sendContainers(MWWorld::CellStore* cellStore)
{
    reset();
    cell = *cellStore->getCell();
    action = BaseEvent::SET;

    MWWorld::CellRefList<ESM::Container> *containerList = cellStore->getContainers();

    for (typename MWWorld::CellRefList<ESM::Container>::List::iterator listIter(containerList->mList.begin());
        listIter != containerList->mList.end(); ++listIter)
    {
        MWWorld::Ptr container(&*listIter, 0);

        mwmp::WorldObject worldObject;
        worldObject.refId = container.getCellRef().getRefId();
        worldObject.refNumIndex = container.getCellRef().getRefNum().mIndex;
        worldObject.mpNum = container.getCellRef().getMpNum();

        MWWorld::ContainerStore& containerStore = container.getClass().getContainerStore(container);

        for (MWWorld::ContainerStoreIterator storeIter = containerStore.begin(); storeIter != containerStore.end(); ++storeIter)
        {
            MWWorld::Ptr itemPtr = *storeIter;

            mwmp::ContainerItem containerItem;
            containerItem.refId = itemPtr.getCellRef().getRefId();
            containerItem.count = itemPtr.getRefData().getCount();
            containerItem.charge = itemPtr.getCellRef().getCharge();

            worldObject.containerChanges.items.push_back(containerItem);
        }

        addObject(worldObject);
    }

    mwmp::Main::get().getNetworking()->getWorldPacket(ID_CONTAINER)->setEvent(this);
    mwmp::Main::get().getNetworking()->getWorldPacket(ID_CONTAINER)->Send();
}


void WorldEvent::sendObjectPlace(MWWorld::Ptr ptr)
{
    reset();
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

    mwmp::Main::get().getNetworking()->getWorldPacket(ID_OBJECT_PLACE)->setEvent(this);
    mwmp::Main::get().getNetworking()->getWorldPacket(ID_OBJECT_PLACE)->Send();

    LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Sending ID_OBJECT_PLACE\n- cellRef: %s, %i\n- count: %i",
        worldObject.refId.c_str(), worldObject.refNumIndex, worldObject.count);
}

void WorldEvent::sendObjectDelete(MWWorld::Ptr ptr)
{
    reset();
    cell = *ptr.getCell()->getCell();

    mwmp::WorldObject worldObject;
    worldObject.refId = ptr.getCellRef().getRefId();
    worldObject.refNumIndex = ptr.getCellRef().getRefNum().mIndex;
    worldObject.mpNum = ptr.getCellRef().getMpNum();
    addObject(worldObject);

    mwmp::Main::get().getNetworking()->getWorldPacket(ID_OBJECT_DELETE)->setEvent(this);
    mwmp::Main::get().getNetworking()->getWorldPacket(ID_OBJECT_DELETE)->Send();
}

void WorldEvent::sendObjectLock(MWWorld::Ptr ptr, int lockLevel)
{
    reset();
    cell = *ptr.getCell()->getCell();

    mwmp::WorldObject worldObject;
    worldObject.refId = ptr.getCellRef().getRefId();
    worldObject.refNumIndex = ptr.getCellRef().getRefNum().mIndex;
    worldObject.mpNum = ptr.getCellRef().getMpNum();
    worldObject.lockLevel = lockLevel;
    addObject(worldObject);

    mwmp::Main::get().getNetworking()->getWorldPacket(ID_OBJECT_LOCK)->setEvent(this);
    mwmp::Main::get().getNetworking()->getWorldPacket(ID_OBJECT_LOCK)->Send();
}

void WorldEvent::sendObjectUnlock(MWWorld::Ptr ptr)
{
    reset();
    cell = *ptr.getCell()->getCell();

    mwmp::WorldObject worldObject;
    worldObject.refId = ptr.getCellRef().getRefId();
    worldObject.refNumIndex = ptr.getCellRef().getRefNum().mIndex;
    worldObject.mpNum = ptr.getCellRef().getMpNum();
    addObject(worldObject);

    mwmp::Main::get().getNetworking()->getWorldPacket(ID_OBJECT_UNLOCK)->setEvent(this);
    mwmp::Main::get().getNetworking()->getWorldPacket(ID_OBJECT_UNLOCK)->Send();
}

void WorldEvent::sendObjectScale(MWWorld::Ptr ptr, float scale)
{
    reset();
    cell = *ptr.getCell()->getCell();

    mwmp::WorldObject worldObject;
    worldObject.refId = ptr.getCellRef().getRefId();
    worldObject.refNumIndex = ptr.getCellRef().getRefNum().mIndex;
    worldObject.mpNum = ptr.getCellRef().getMpNum();
    worldObject.scale = scale;
    addObject(worldObject);

    mwmp::Main::get().getNetworking()->getWorldPacket(ID_OBJECT_SCALE)->setEvent(this);
    mwmp::Main::get().getNetworking()->getWorldPacket(ID_OBJECT_SCALE)->Send();
}

void WorldEvent::sendObjectAnimPlay(MWWorld::Ptr ptr, std::string group, int mode)
{
    reset();
    cell = *ptr.getCell()->getCell();

    mwmp::WorldObject worldObject;
    worldObject.refId = ptr.getCellRef().getRefId();
    worldObject.refNumIndex = ptr.getCellRef().getRefNum().mIndex;
    worldObject.mpNum = ptr.getCellRef().getMpNum();
    worldObject.animGroup = group;
    worldObject.animMode = mode;
    addObject(worldObject);

    mwmp::Main::get().getNetworking()->getWorldPacket(ID_OBJECT_ANIM_PLAY)->setEvent(this);
    mwmp::Main::get().getNetworking()->getWorldPacket(ID_OBJECT_ANIM_PLAY)->Send();
}

void WorldEvent::sendDoorState(MWWorld::Ptr ptr, int state)
{
    reset();
    cell = *ptr.getCell()->getCell();

    mwmp::WorldObject worldObject;
    worldObject.refId = ptr.getCellRef().getRefId();
    worldObject.refNumIndex = ptr.getCellRef().getRefNum().mIndex;
    worldObject.mpNum = ptr.getCellRef().getMpNum();
    worldObject.doorState = state;
    addObject(worldObject);

    mwmp::Main::get().getNetworking()->getWorldPacket(ID_DOOR_STATE)->setEvent(this);
    mwmp::Main::get().getNetworking()->getWorldPacket(ID_DOOR_STATE)->Send();

    LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Door activation 1\n- cellRef: %s, %i\n- cell: %s\n- state: %s",
        worldObject.refId.c_str(), worldObject.refNumIndex, cell.getDescription().c_str(),
        worldObject.doorState ? "true" : "false");
}

void WorldEvent::sendMusicPlay(std::string filename)
{
    reset();
    mwmp::WorldObject worldObject;
    worldObject.filename = filename;
    addObject(worldObject);

    mwmp::Main::get().getNetworking()->getWorldPacket(ID_MUSIC_PLAY)->setEvent(this);
    mwmp::Main::get().getNetworking()->getWorldPacket(ID_MUSIC_PLAY)->Send();
}

void WorldEvent::sendVideoPlay(std::string filename, bool allowSkipping)
{
    reset();
    mwmp::WorldObject worldObject;
    worldObject.filename = filename;
    worldObject.allowSkipping = allowSkipping;
    addObject(worldObject);

    mwmp::Main::get().getNetworking()->getWorldPacket(ID_VIDEO_PLAY)->setEvent(this);
    mwmp::Main::get().getNetworking()->getWorldPacket(ID_VIDEO_PLAY)->Send();
}

void WorldEvent::sendScriptLocalShort(MWWorld::Ptr ptr, int index, int shortVal)
{
    reset();
    cell = *ptr.getCell()->getCell();

    mwmp::WorldObject worldObject;
    worldObject.refId = ptr.getCellRef().getRefId();
    worldObject.refNumIndex = ptr.getCellRef().getRefNum().mIndex;
    worldObject.mpNum = ptr.getCellRef().getMpNum();
    worldObject.index = index;
    worldObject.shortVal = shortVal;
    addObject(worldObject);

    mwmp::Main::get().getNetworking()->getWorldPacket(ID_SCRIPT_LOCAL_SHORT)->setEvent(this);
    mwmp::Main::get().getNetworking()->getWorldPacket(ID_SCRIPT_LOCAL_SHORT)->Send();

    LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Sending ID_SCRIPT_LOCAL_SHORT\n- cellRef: %s, %i\n- cell: %s\n- index: %i\n- shortVal: %i",
        worldObject.refId.c_str(), worldObject.refNumIndex, cell.getDescription().c_str(),
        worldObject.index, worldObject.shortVal);
}

void WorldEvent::sendScriptLocalFloat(MWWorld::Ptr ptr, int index, float floatVal)
{
    reset();
    cell = *ptr.getCell()->getCell();

    mwmp::WorldObject worldObject;
    worldObject.refId = ptr.getCellRef().getRefId();
    worldObject.refNumIndex = ptr.getCellRef().getRefNum().mIndex;
    worldObject.mpNum = ptr.getCellRef().getMpNum();
    worldObject.index = index;
    worldObject.floatVal = floatVal;
    addObject(worldObject);

    mwmp::Main::get().getNetworking()->getWorldPacket(ID_SCRIPT_LOCAL_FLOAT)->setEvent(this);
    mwmp::Main::get().getNetworking()->getWorldPacket(ID_SCRIPT_LOCAL_FLOAT)->Send();

    LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Sending ID_SCRIPT_LOCAL_FLOAT\n- cellRef: %s, %i\n- cell: %s\n- index: %i\n- floatVal: %f",
        worldObject.refId.c_str(), worldObject.refNumIndex, cell.getDescription().c_str(),
        worldObject.index, worldObject.floatVal);
}

void WorldEvent::sendScriptMemberShort(std::string refId, int index, int shortVal)
{
    reset();
    mwmp::WorldObject worldObject;
    worldObject.refId = refId;
    worldObject.index = index;
    worldObject.shortVal = shortVal;
    addObject(worldObject);

    mwmp::Main::get().getNetworking()->getWorldPacket(ID_SCRIPT_MEMBER_SHORT)->setEvent(this);
    mwmp::Main::get().getNetworking()->getWorldPacket(ID_SCRIPT_MEMBER_SHORT)->Send();

    LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Sending ID_SCRIPT_MEMBER_SHORT\n- cellRef: %s\n- index: %i\n- shortVal: %i",
        worldObject.refId.c_str(), worldObject.index, worldObject.shortVal);
}

void WorldEvent::sendScriptGlobalShort(std::string varName, int shortVal)
{
    reset();
    mwmp::WorldObject worldObject;
    worldObject.varName = varName;
    worldObject.shortVal = shortVal;
    addObject(worldObject);

    mwmp::Main::get().getNetworking()->getWorldPacket(ID_SCRIPT_GLOBAL_SHORT)->setEvent(this);
    mwmp::Main::get().getNetworking()->getWorldPacket(ID_SCRIPT_GLOBAL_SHORT)->Send();

    LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Sending ID_SCRIPT_GLOBAL_SHORT\n- varName: %s\n- shortVal: %i",
        worldObject.varName.c_str(), worldObject.shortVal);
}
