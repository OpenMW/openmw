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

void WorldEvent::addObject(WorldObject worldObject)
{
    objectChanges.objects.push_back(worldObject);
}

void WorldEvent::sendContainers(MWWorld::CellStore* cellStore)
{
    mwmp::WorldEvent *worldEvent = mwmp::Main::get().getNetworking()->resetWorldEvent();
    worldEvent->cell = *cellStore->getCell();
    worldEvent->action = BaseEvent::SET;

    MWWorld::CellRefList<ESM::Container> *containerList = cellStore->getContainers();

    for (typename MWWorld::CellRefList<ESM::Container>::List::iterator listIter(containerList->mList.begin());
        listIter != containerList->mList.end(); ++listIter)
    {
        MWWorld::Ptr container(&*listIter, 0);

        mwmp::WorldObject worldObject;
        worldObject.refId = container.getCellRef().getRefId();
        worldObject.refNumIndex = container.getCellRef().getRefNum().mIndex;

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

        worldEvent->addObject(worldObject);
    }

    mwmp::Main::get().getNetworking()->getWorldPacket(ID_CONTAINER)->Send(worldEvent);
}

void WorldEvent::editContainers(MWWorld::CellStore* cellStore)
{
    WorldObject worldObject;

    for (unsigned int i = 0; i < objectChanges.count; i++)
    {
        worldObject = objectChanges.objects.at(i);

        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s, %i",
            worldObject.refId.c_str(),
            worldObject.refNumIndex);

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refId, worldObject.refNumIndex);

        if (ptrFound)
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Found %s, %i",
                ptrFound.getCellRef().getRefId().c_str(),
                ptrFound.getCellRef().getRefNum());

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

        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s, %i\n- charge: %i\n- count: %i",
            worldObject.refId.c_str(),
            worldObject.refNumIndex,
            worldObject.charge,
            worldObject.count);

        MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(), worldObject.refId, 1);
        MWWorld::Ptr newPtr = ref.getPtr();

        if (worldObject.charge > -1)
            newPtr.getCellRef().setCharge(worldObject.charge);

        if (worldObject.count > 1)
            newPtr.getRefData().setCount(worldObject.count);

        newPtr.getCellRef().setGoldValue(worldObject.goldValue);
        newPtr = MWBase::Environment::get().getWorld()->placeObject(newPtr, cellStore, worldObject.pos);

        // Change RefNum here because the line above unsets it
        newPtr.getCellRef().setRefNumIndex(worldObject.refNumIndex);

        // If this RefNum is higher than the last we've recorded for this CellStore,
        // start using it as our new last one
        if (cellStore->getLastRefNumIndex() < worldObject.refNumIndex)
            cellStore->setLastRefNumIndex(worldObject.refNumIndex);
    }
}

void WorldEvent::deleteObjects(MWWorld::CellStore* cellStore)
{
    WorldObject worldObject;

    for (unsigned int i = 0; i < objectChanges.count; i++)
    {
        worldObject = objectChanges.objects.at(i);

        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s, %i",
            worldObject.refId.c_str(),
            worldObject.refNumIndex);

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refId, worldObject.refNumIndex);

        if (ptrFound)
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Found %s, %i",
                ptrFound.getCellRef().getRefId().c_str(),
                ptrFound.getCellRef().getRefNum());

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

        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s, %i",
            worldObject.refId.c_str(),
            worldObject.refNumIndex);

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refId, worldObject.refNumIndex);

        if (ptrFound)
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Found %s, %i",
                ptrFound.getCellRef().getRefId().c_str(),
                ptrFound.getCellRef().getRefNum());

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

        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s, %i",
            worldObject.refId.c_str(),
            worldObject.refNumIndex);

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refId, worldObject.refNumIndex);

        if (ptrFound)
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Found %s, %i",
                ptrFound.getCellRef().getRefId().c_str(),
                ptrFound.getCellRef().getRefNum());

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

        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s, %i",
            worldObject.refId.c_str(),
            worldObject.refNumIndex);

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refId, worldObject.refNumIndex);

        if (ptrFound)
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Found %s, %i",
                ptrFound.getCellRef().getRefId().c_str(),
                ptrFound.getCellRef().getRefNum());

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

        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s, %i",
            worldObject.refId.c_str(),
            worldObject.refNumIndex);

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refId, worldObject.refNumIndex);

        if (ptrFound)
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Found %s, %i",
                ptrFound.getCellRef().getRefId().c_str(),
                ptrFound.getCellRef().getRefNum());

            MWBase::Environment::get().getWorld()->moveObject(ptrFound,
                worldObject.pos.pos[0], worldObject.pos.pos[1], worldObject.pos.pos[2]);
        }
    }
}

void WorldEvent::rotateObjects(MWWorld::CellStore* cellStore)
{
    WorldObject worldObject;

    for (unsigned int i = 0; i < objectChanges.count; i++)
    {
        worldObject = objectChanges.objects.at(i);

        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s, %i",
            worldObject.refId.c_str(),
            worldObject.refNumIndex);

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refId, worldObject.refNumIndex);

        if (ptrFound)
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Found %s, %i",
                ptrFound.getCellRef().getRefId().c_str(),
                ptrFound.getCellRef().getRefNum());

            MWBase::Environment::get().getWorld()->rotateObject(ptrFound,
                worldObject.pos.rot[0], worldObject.pos.rot[1], worldObject.pos.rot[2]);
        }
    }
}

void WorldEvent::animateObjects(MWWorld::CellStore* cellStore)
{
    WorldObject worldObject;

    for (unsigned int i = 0; i < objectChanges.count; i++)
    {
        worldObject = objectChanges.objects.at(i);

        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s, %i",
            worldObject.refId.c_str(),
            worldObject.refNumIndex);

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refId, worldObject.refNumIndex);

        if (ptrFound)
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Found %s, %i",
                ptrFound.getCellRef().getRefId().c_str(),
                ptrFound.getCellRef().getRefNum());

            MWBase::Environment::get().getMechanicsManager()->playAnimationGroup(ptrFound,
                worldObject.animGroup, worldObject.animMode, std::numeric_limits<int>::max(), true);
        }
    }
}

void WorldEvent::activateDoors(MWWorld::CellStore* cellStore)
{
    WorldObject worldObject;

    for (unsigned int i = 0; i < objectChanges.count; i++)
    {
        worldObject = objectChanges.objects.at(i);

        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s, %i",
            worldObject.refId.c_str(),
            worldObject.refNumIndex);

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refId, worldObject.refNumIndex);

        if (ptrFound)
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Found %s, %i",
                ptrFound.getCellRef().getRefId().c_str(),
                ptrFound.getCellRef().getRefNum());

            ptrFound.getClass().setDoorState(ptrFound, worldObject.doorState);
            MWBase::Environment::get().getWorld()->saveDoorState(ptrFound, worldObject.doorState);
        }
    }
}

void WorldEvent::playMusic()
{
    WorldObject worldObject;

    for (unsigned int i = 0; i < objectChanges.count; i++)
    {
        worldObject = objectChanges.objects.at(i);

        LOG_APPEND(Log::LOG_VERBOSE, "- filename: %s",
            worldObject.filename.c_str());

        MWBase::Environment::get().getSoundManager()->streamMusic(worldObject.filename);
    }
}

void WorldEvent::playVideo()
{
    WorldObject worldObject;

    for (unsigned int i = 0; i < objectChanges.count; i++)
    {
        worldObject = objectChanges.objects.at(i);

        LOG_APPEND(Log::LOG_VERBOSE, "- filename: %s\n- allowSkipping: %s",
            worldObject.filename.c_str(),
            worldObject.allowSkipping ? "true" : "false");

        MWBase::Environment::get().getWindowManager()->playVideo(worldObject.filename, worldObject.allowSkipping);
    }
}

void WorldEvent::setLocalShorts(MWWorld::CellStore* cellStore)
{
    WorldObject worldObject;

    for (unsigned int i = 0; i < objectChanges.count; i++)
    {
        worldObject = objectChanges.objects.at(i);

        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s, %i\n- index: %i\n- shortVal: %i",
            worldObject.refId.c_str(),
            worldObject.refNumIndex,
            worldObject.index,
            worldObject.shortVal);

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refId, worldObject.refNumIndex);

        if (ptrFound)
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Found %s, %i",
                ptrFound.getCellRef().getRefId().c_str(),
                ptrFound.getCellRef().getRefNum());

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

        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s, %i\n- index: %i\n- floatVal: %f",
            worldObject.refId.c_str(),
            worldObject.refNumIndex,
            worldObject.index,
            worldObject.floatVal);

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refId, worldObject.refNumIndex);

        if (ptrFound)
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Found %s, %i",
                ptrFound.getCellRef().getRefId().c_str(),
                ptrFound.getCellRef().getRefNum());

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

        LOG_APPEND(Log::LOG_VERBOSE, "- cellRef: %s\n- index: %i\n- shortVal: %i\n",
            worldObject.refId.c_str(),
            worldObject.index,
            worldObject.shortVal);

        // Mimic the way a Ptr is fetched in InterpreterContext for similar situations
        MWWorld::Ptr ptrFound = MWBase::Environment::get().getWorld()->searchPtr(worldObject.refId, false);

        if (!ptrFound.isEmpty())
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Found %s, %i",
                ptrFound.getCellRef().getRefId().c_str(),
                ptrFound.getCellRef().getRefNum());

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

        LOG_APPEND(Log::LOG_VERBOSE, "- varName: %s\n- shortVal: %i",
            worldObject.varName.c_str(),
            worldObject.shortVal);

        MWBase::Environment::get().getWorld()->setGlobalInt(worldObject.varName, worldObject.shortVal);
    }
}
