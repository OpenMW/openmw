#include "LocalEvent.hpp"
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

LocalEvent::LocalEvent(RakNet::RakNetGUID guid)
{
    this->guid = guid;
}

LocalEvent::~LocalEvent()
{

}

Networking *LocalEvent::getNetworking()
{
    return mwmp::Main::get().getNetworking();
}

void LocalEvent::addObject(WorldObject worldObject)
{
    objectChanges.objects.push_back(worldObject);
}

void LocalEvent::addContainerItem(ContainerItem containerItem)
{
    containerChanges.items.push_back(containerItem);
}

void LocalEvent::editContainer(MWWorld::CellStore* cellStore)
{
    WorldObject worldObject;

    for (unsigned int i = 0; i < objectChanges.count; i++)
    {
        worldObject = objectChanges.objects.at(i);

        LOG_APPEND(Log::LOG_WARN, "- cellRef: %s, %i\n- cell: %s",
            worldObject.refId.c_str(),
            worldObject.refNumIndex,
            cell.getDescription().c_str());

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refId, worldObject.refNumIndex);

        if (ptrFound)
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Found %s, %i",
                ptrFound.getCellRef().getRefId().c_str(),
                ptrFound.getCellRef().getRefNum());

            MWWorld::ContainerStore& containerStore = ptrFound.getClass().getContainerStore(ptrFound);

            for (unsigned int i = 0; i < containerChanges.count; i++)
            {
                ContainerItem item = containerChanges.items.at(i);

                if (containerChanges.action == ContainerChanges::ADD)
                {
                    containerStore.add(item.refId, item.count, mwmp::Players::getPlayer(guid)->getPtr());
                }
                else if (containerChanges.action == ContainerChanges::REMOVE)
                {
                    containerStore.remove(item.refId, item.count, mwmp::Players::getPlayer(guid)->getPtr());
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

void LocalEvent::placeObjects(MWWorld::CellStore* cellStore)
{
    WorldObject worldObject;

    for (unsigned int i = 0; i < objectChanges.count; i++)
    {
        worldObject = objectChanges.objects.at(i);

        LOG_APPEND(Log::LOG_WARN, "- cellRef: %s, %i\n- cell: %s\n- charge: %i\n- count: %i",
            worldObject.refId.c_str(),
            worldObject.refNumIndex,
            cell.getDescription().c_str(),
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

void LocalEvent::deleteObjects(MWWorld::CellStore* cellStore)
{
    WorldObject worldObject;

    for (unsigned int i = 0; i < objectChanges.count; i++)
    {
        worldObject = objectChanges.objects.at(i);

        LOG_APPEND(Log::LOG_WARN, "- cellRef: %s, %i\n- cell: %s",
            worldObject.refId.c_str(),
            worldObject.refNumIndex,
            cell.getDescription().c_str());

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refId, worldObject.refNumIndex);

        if (ptrFound)
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Found %s, %i",
                ptrFound.getCellRef().getRefId().c_str(),
                ptrFound.getCellRef().getRefNum());

            MWBase::Environment::get().getWorld()->deleteObject(ptrFound);
        }
    }
}

void LocalEvent::lockObjects(MWWorld::CellStore* cellStore)
{
    WorldObject worldObject;

    for (unsigned int i = 0; i < objectChanges.count; i++)
    {
        worldObject = objectChanges.objects.at(i);

        LOG_APPEND(Log::LOG_WARN, "- cellRef: %s, %i\n- cell: %s",
            worldObject.refId.c_str(),
            worldObject.refNumIndex,
            cell.getDescription().c_str());

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refId, worldObject.refNumIndex);

        if (ptrFound)
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Found %s, %i",
                ptrFound.getCellRef().getRefId().c_str(),
                ptrFound.getCellRef().getRefNum());

            ptrFound.getClass().lock(ptrFound, worldObject.lockLevel);
        }
    }
}

void LocalEvent::unlockObjects(MWWorld::CellStore* cellStore)
{
    WorldObject worldObject;

    for (unsigned int i = 0; i < objectChanges.count; i++)
    {
        worldObject = objectChanges.objects.at(i);

        LOG_APPEND(Log::LOG_WARN, "- cellRef: %s, %i\n- cell: %s",
            worldObject.refId.c_str(),
            worldObject.refNumIndex,
            cell.getDescription().c_str());

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refId, worldObject.refNumIndex);

        if (ptrFound)
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Found %s, %i",
                ptrFound.getCellRef().getRefId().c_str(),
                ptrFound.getCellRef().getRefNum());

            ptrFound.getClass().unlock(ptrFound);
        }
    }
}

void LocalEvent::scaleObjects(MWWorld::CellStore* cellStore)
{
    WorldObject worldObject;

    for (unsigned int i = 0; i < objectChanges.count; i++)
    {
        worldObject = objectChanges.objects.at(i);

        LOG_APPEND(Log::LOG_WARN, "- cellRef: %s, %i\n- cell: %s",
            worldObject.refId.c_str(),
            worldObject.refNumIndex,
            cell.getDescription().c_str());

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refId, worldObject.refNumIndex);

        if (ptrFound)
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Found %s, %i",
                ptrFound.getCellRef().getRefId().c_str(),
                ptrFound.getCellRef().getRefNum());

            MWBase::Environment::get().getWorld()->scaleObject(ptrFound, worldObject.scale);
        }
    }
}

void LocalEvent::moveObjects(MWWorld::CellStore* cellStore)
{
    WorldObject worldObject;

    for (unsigned int i = 0; i < objectChanges.count; i++)
    {
        worldObject = objectChanges.objects.at(i);

        LOG_APPEND(Log::LOG_WARN, "- cellRef: %s, %i\n- cell: %s",
            worldObject.refId.c_str(),
            worldObject.refNumIndex,
            cell.getDescription().c_str());

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refId, worldObject.refNumIndex);

        if (ptrFound)
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Found %s, %i",
                ptrFound.getCellRef().getRefId().c_str(),
                ptrFound.getCellRef().getRefNum());

            MWBase::Environment::get().getWorld()->moveObject(ptrFound,
                worldObject.pos.pos[0], worldObject.pos.pos[1], worldObject.pos.pos[2]);
        }
    }
}

void LocalEvent::rotateObjects(MWWorld::CellStore* cellStore)
{
    WorldObject worldObject;

    for (unsigned int i = 0; i < objectChanges.count; i++)
    {
        worldObject = objectChanges.objects.at(i);

        LOG_APPEND(Log::LOG_WARN, "- cellRef: %s, %i\n- cell: %s",
            worldObject.refId.c_str(),
            worldObject.refNumIndex,
            cell.getDescription().c_str());

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refId, worldObject.refNumIndex);

        if (ptrFound)
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Found %s, %i",
                ptrFound.getCellRef().getRefId().c_str(),
                ptrFound.getCellRef().getRefNum());

            MWBase::Environment::get().getWorld()->rotateObject(ptrFound,
                worldObject.pos.rot[0], worldObject.pos.rot[1], worldObject.pos.rot[2]);
        }
    }
}

void LocalEvent::animateObjects(MWWorld::CellStore* cellStore)
{
    WorldObject worldObject;

    for (unsigned int i = 0; i < objectChanges.count; i++)
    {
        worldObject = objectChanges.objects.at(i);

        LOG_APPEND(Log::LOG_WARN, "- cellRef: %s, %i\n- cell: %s",
            worldObject.refId.c_str(),
            worldObject.refNumIndex,
            cell.getDescription().c_str());

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refId, worldObject.refNumIndex);

        if (ptrFound)
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Found %s, %i",
                ptrFound.getCellRef().getRefId().c_str(),
                ptrFound.getCellRef().getRefNum());

            MWBase::Environment::get().getMechanicsManager()->playAnimationGroup(ptrFound,
                worldObject.animGroup, worldObject.animMode, std::numeric_limits<int>::max(), true);
        }
    }
}

void LocalEvent::activateDoors(MWWorld::CellStore* cellStore)
{
    WorldObject worldObject;

    for (unsigned int i = 0; i < objectChanges.count; i++)
    {
        worldObject = objectChanges.objects.at(i);

        LOG_APPEND(Log::LOG_WARN, "- cellRef: %s, %i\n- cell: %s",
            worldObject.refId.c_str(),
            worldObject.refNumIndex,
            cell.getDescription().c_str());

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refId, worldObject.refNumIndex);

        if (ptrFound)
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Found %s, %i",
                ptrFound.getCellRef().getRefId().c_str(),
                ptrFound.getCellRef().getRefNum());

            ptrFound.getClass().setDoorState(ptrFound, worldObject.state);
            MWBase::Environment::get().getWorld()->saveDoorState(ptrFound, worldObject.state);
        }
    }
}

void LocalEvent::playMusic()
{
    WorldObject worldObject;

    for (unsigned int i = 0; i < objectChanges.count; i++)
    {
        worldObject = objectChanges.objects.at(i);

        LOG_APPEND(Log::LOG_WARN, "- filename: %s",
            worldObject.filename.c_str());

        MWBase::Environment::get().getSoundManager()->streamMusic(worldObject.filename);
    }
}

void LocalEvent::playVideo()
{
    WorldObject worldObject;

    for (unsigned int i = 0; i < objectChanges.count; i++)
    {
        worldObject = objectChanges.objects.at(i);

        LOG_APPEND(Log::LOG_WARN, "- filename: %s\n- allowSkipping: %s",
            worldObject.filename.c_str(),
            worldObject.allowSkipping ? "true" : "false");

        MWBase::Environment::get().getWindowManager()->playVideo(worldObject.filename, worldObject.allowSkipping);
    }
}

void LocalEvent::setLocalShorts(MWWorld::CellStore* cellStore)
{
    WorldObject worldObject;

    for (unsigned int i = 0; i < objectChanges.count; i++)
    {
        worldObject = objectChanges.objects.at(i);

        LOG_APPEND(Log::LOG_WARN, "- cellRef: %s, %i\n- cell: %s\n- index: %i\n- shortVal: %i",
            worldObject.refId.c_str(),
            worldObject.refNumIndex,
            cell.getDescription().c_str(),
            worldObject.index,
            worldObject.shortVal);

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refId, worldObject.refNumIndex);

        if (ptrFound)
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Found %s, %i",
                ptrFound.getCellRef().getRefId().c_str(),
                ptrFound.getCellRef().getRefNum());

            ptrFound.getRefData().getLocals().mShorts.at(worldObject.index) = worldObject.shortVal;
        }
    }
}

void LocalEvent::setLocalFloats(MWWorld::CellStore* cellStore)
{
    WorldObject worldObject;

    for (unsigned int i = 0; i < objectChanges.count; i++)
    {
        worldObject = objectChanges.objects.at(i);

        LOG_APPEND(Log::LOG_WARN, "- cellRef: %s, %i\n- cell: %s\n- index: %i\n- floatVal: %f",
            worldObject.refId.c_str(),
            worldObject.refNumIndex,
            cell.getDescription().c_str(),
            worldObject.index,
            worldObject.floatVal);

        MWWorld::Ptr ptrFound = cellStore->searchExact(worldObject.refId, worldObject.refNumIndex);

        if (ptrFound)
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Found %s, %i",
                ptrFound.getCellRef().getRefId().c_str(),
                ptrFound.getCellRef().getRefNum());

            ptrFound.getRefData().getLocals().mFloats.at(worldObject.index) = worldObject.floatVal;
        }
    }
}

void LocalEvent::setMemberShorts()
{
    WorldObject worldObject;

    for (unsigned int i = 0; i < objectChanges.count; i++)
    {
        worldObject = objectChanges.objects.at(i);

        LOG_APPEND(Log::LOG_WARN, "- cellRef: %s\n- index: %i\n- shortVal: %i\n",
            worldObject.refId.c_str(),
            worldObject.index,
            worldObject.shortVal);

        // Mimic the way a Ptr is fetched in InterpreterContext for similar situations
        MWWorld::Ptr ptrFound = MWBase::Environment::get().getWorld()->getPtr(worldObject.refId, false);

        if (ptrFound)
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Found %s, %i",
                ptrFound.getCellRef().getRefId().c_str(),
                ptrFound.getCellRef().getRefNum());

            std::string scriptId = ptrFound.getClass().getScript(ptrFound);

            ptrFound.getRefData().setLocals(
                *MWBase::Environment::get().getWorld()->getStore().get<ESM::Script>().find(scriptId));

            ptrFound.getRefData().getLocals().mShorts.at(worldObject.index) = worldObject.shortVal;;
        }
    }
}

void LocalEvent::setGlobalShorts()
{
    WorldObject worldObject;

    for (unsigned int i = 0; i < objectChanges.count; i++)
    {
        worldObject = objectChanges.objects.at(i);

        LOG_APPEND(Log::LOG_WARN, "- varName: %s\n- shortVal: %i",
            worldObject.varName.c_str(),
            worldObject.shortVal);

        MWBase::Environment::get().getWorld()->setGlobalInt(worldObject.varName, worldObject.shortVal);
    }
}
