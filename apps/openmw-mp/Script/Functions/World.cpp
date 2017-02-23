#include <regex>

#include <apps/openmw-mp/Player.hpp>
#include <apps/openmw-mp/Script/ScriptFunctions.hpp>
#include <apps/openmw-mp/Networking.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Base/BaseEvent.hpp>
#include "World.hpp"

using namespace mwmp;

BaseEvent scriptEvent;
WorldObject tempWorldObject;
ContainerItem tempContainerItem;

std::regex exteriorCellPattern("^(-?\\d+), (-?\\d+)$");

void WorldFunctions::InitScriptEvent(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    scriptEvent.cell.blank();
    scriptEvent.objectChanges.objects.clear();
    scriptEvent.guid = player->guid;
}

unsigned int WorldFunctions::GetObjectChangesSize() noexcept
{
    return mwmp::Networking::getPtr()->getLastEvent()->objectChanges.count;
}

unsigned char WorldFunctions::GetLastEventAction() noexcept
{
    return mwmp::Networking::getPtr()->getLastEvent()->action;
}

const char *WorldFunctions::GetObjectRefId(unsigned int i) noexcept
{
    return mwmp::Networking::getPtr()->getLastEvent()->objectChanges.objects.at(i).refId.c_str();
}

int WorldFunctions::GetObjectRefNumIndex(unsigned int i) noexcept
{
    return mwmp::Networking::getPtr()->getLastEvent()->objectChanges.objects.at(i).refNumIndex;
}

int WorldFunctions::GetObjectCount(unsigned int i) noexcept
{
    return mwmp::Networking::getPtr()->getLastEvent()->objectChanges.objects.at(i).count;
}

int WorldFunctions::GetObjectCharge(unsigned int i) noexcept
{
    return mwmp::Networking::getPtr()->getLastEvent()->objectChanges.objects.at(i).charge;
}

int WorldFunctions::GetObjectGoldValue(unsigned int i) noexcept
{
    return mwmp::Networking::getPtr()->getLastEvent()->objectChanges.objects.at(i).goldValue;
}

double WorldFunctions::GetObjectScale(unsigned int i) noexcept
{
    return mwmp::Networking::getPtr()->getLastEvent()->objectChanges.objects.at(i).scale;
}

int WorldFunctions::GetObjectDoorState(unsigned int i) noexcept
{
    return mwmp::Networking::getPtr()->getLastEvent()->objectChanges.objects.at(i).doorState;
}

int WorldFunctions::GetObjectLockLevel(unsigned int i) noexcept
{
    return mwmp::Networking::getPtr()->getLastEvent()->objectChanges.objects.at(i).lockLevel;
}

double WorldFunctions::GetObjectPosX(unsigned int i) noexcept
{
    return mwmp::Networking::getPtr()->getLastEvent()->objectChanges.objects.at(i).pos.pos[0];
}

double WorldFunctions::GetObjectPosY(unsigned int i) noexcept
{
    return mwmp::Networking::getPtr()->getLastEvent()->objectChanges.objects.at(i).pos.pos[1];
}

double WorldFunctions::GetObjectPosZ(unsigned int i) noexcept
{
    return mwmp::Networking::getPtr()->getLastEvent()->objectChanges.objects.at(i).pos.pos[2];
}

double WorldFunctions::GetObjectRotX(unsigned int i) noexcept
{
    return mwmp::Networking::getPtr()->getLastEvent()->objectChanges.objects.at(i).pos.rot[0];
}

double WorldFunctions::GetObjectRotY(unsigned int i) noexcept
{
    return mwmp::Networking::getPtr()->getLastEvent()->objectChanges.objects.at(i).pos.rot[1];
}

double WorldFunctions::GetObjectRotZ(unsigned int i) noexcept
{
    return mwmp::Networking::getPtr()->getLastEvent()->objectChanges.objects.at(i).pos.rot[2];
}

unsigned int WorldFunctions::GetContainerChangesSize(unsigned int objectIndex) noexcept
{
    return mwmp::Networking::getPtr()->getLastEvent()->objectChanges.objects.at(objectIndex).containerChanges.count;
}

const char *WorldFunctions::GetContainerItemRefId(unsigned int objectIndex, unsigned int itemIndex) noexcept
{
    return mwmp::Networking::getPtr()->getLastEvent()->objectChanges.objects.at(objectIndex)
        .containerChanges.items.at(itemIndex).refId.c_str();
}

int WorldFunctions::GetContainerItemCount(unsigned int objectIndex, unsigned int itemIndex) noexcept
{
    return mwmp::Networking::getPtr()->getLastEvent()->objectChanges.objects.at(objectIndex)
        .containerChanges.items.at(itemIndex).count;
}

int WorldFunctions::GetContainerItemCharge(unsigned int objectIndex, unsigned int itemIndex) noexcept
{
    return mwmp::Networking::getPtr()->getLastEvent()->objectChanges.objects.at(objectIndex)
        .containerChanges.items.at(itemIndex).charge;
}

int WorldFunctions::GetContainerItemActionCount(unsigned int objectIndex, unsigned int itemIndex) noexcept
{
    return mwmp::Networking::getPtr()->getLastEvent()->objectChanges.objects.at(objectIndex)
        .containerChanges.items.at(itemIndex).actionCount;
}

void WorldFunctions::SetScriptEventCell(const char* cellDescription) noexcept
{
    std::string description = cellDescription;
    std::smatch baseMatch;

    if (std::regex_match(description, baseMatch, exteriorCellPattern))
    {
        scriptEvent.cell.mData.mFlags &= ~ESM::Cell::Interior;

        // The first sub match is the whole string, so check for a length of 3
        if (baseMatch.size() == 3)
        {
            scriptEvent.cell.mData.mX = stoi(baseMatch[1].str());
            scriptEvent.cell.mData.mY = stoi(baseMatch[2].str());
        }
    }
    else
    {
        scriptEvent.cell.mData.mFlags |= ESM::Cell::Interior;
        scriptEvent.cell.mName = description;
    }
}

void WorldFunctions::SetScriptEventAction(unsigned char action) noexcept
{
    scriptEvent.action = action;
}

void WorldFunctions::SetObjectRefId(const char* refId) noexcept
{
    tempWorldObject.refId = refId;
}

void WorldFunctions::SetObjectRefNumIndex(int refNumIndex) noexcept
{
    tempWorldObject.refNumIndex = refNumIndex;
}

void WorldFunctions::SetObjectCount(int count) noexcept
{
    tempWorldObject.count = count;
}

void WorldFunctions::SetObjectCharge(int charge) noexcept
{
    tempWorldObject.charge = charge;
}

void WorldFunctions::SetObjectGoldValue(int goldValue) noexcept
{
    tempWorldObject.goldValue = goldValue;
}

void WorldFunctions::SetObjectScale(double scale) noexcept
{
    tempWorldObject.scale = scale;
}

void WorldFunctions::SetObjectDoorState(int doorState) noexcept
{
    tempWorldObject.doorState = doorState;
}

void WorldFunctions::SetObjectLockLevel(int lockLevel) noexcept
{
    tempWorldObject.lockLevel = lockLevel;
}

void WorldFunctions::SetObjectPosition(double x, double y, double z) noexcept
{
    tempWorldObject.pos.pos[0] = x;
    tempWorldObject.pos.pos[1] = y;
    tempWorldObject.pos.pos[2] = z;
}

void WorldFunctions::SetObjectRotation(double x, double y, double z) noexcept
{
    tempWorldObject.pos.rot[0] = x;
    tempWorldObject.pos.rot[1] = y;
    tempWorldObject.pos.rot[2] = z;
}

void WorldFunctions::SetContainerItemRefId(const char* refId) noexcept
{
    tempContainerItem.refId = refId;
}

void WorldFunctions::SetContainerItemCount(int count) noexcept
{
    tempContainerItem.count = count;
}

void WorldFunctions::SetContainerItemCharge(int charge) noexcept
{
    tempContainerItem.charge = charge;
}

void WorldFunctions::AddWorldObject() noexcept
{
    WorldObject worldObject;
    worldObject.refId = tempWorldObject.refId;
    worldObject.refNumIndex = tempWorldObject.refNumIndex;
    worldObject.count = tempWorldObject.count;
    worldObject.charge = tempWorldObject.charge;
    worldObject.goldValue = tempWorldObject.goldValue;
    worldObject.scale = tempWorldObject.scale;
    worldObject.doorState = tempWorldObject.doorState;
    worldObject.lockLevel = tempWorldObject.lockLevel;
    worldObject.pos = tempWorldObject.pos;
    worldObject.containerChanges.items = tempWorldObject.containerChanges.items;

    scriptEvent.objectChanges.objects.push_back(worldObject);

    tempWorldObject.containerChanges.items.clear();
}

void WorldFunctions::AddContainerItem() noexcept
{
    ContainerItem containerItem;
    containerItem.refId = tempContainerItem.refId;
    containerItem.count = tempContainerItem.count;
    containerItem.charge = tempContainerItem.charge;

    tempWorldObject.containerChanges.items.push_back(containerItem);
}

void WorldFunctions::SendObjectDelete() noexcept
{
    mwmp::Networking::get().getWorldController()->GetPacket(ID_OBJECT_DELETE)->Send(&scriptEvent, scriptEvent.guid);
}

void WorldFunctions::SendObjectPlace() noexcept
{
    mwmp::Networking::get().getWorldController()->GetPacket(ID_OBJECT_PLACE)->Send(&scriptEvent, scriptEvent.guid);
}

void WorldFunctions::SendObjectScale() noexcept
{
    mwmp::Networking::get().getWorldController()->GetPacket(ID_OBJECT_SCALE)->Send(&scriptEvent, scriptEvent.guid);
}

void WorldFunctions::SendObjectLock() noexcept
{
    mwmp::Networking::get().getWorldController()->GetPacket(ID_OBJECT_LOCK)->Send(&scriptEvent, scriptEvent.guid);
}

void WorldFunctions::SendObjectUnlock() noexcept
{
    mwmp::Networking::get().getWorldController()->GetPacket(ID_OBJECT_UNLOCK)->Send(&scriptEvent, scriptEvent.guid);
}

void WorldFunctions::SendDoorState() noexcept
{
    mwmp::Networking::get().getWorldController()->GetPacket(ID_DOOR_STATE)->Send(&scriptEvent, scriptEvent.guid);
}

void WorldFunctions::SendContainer() noexcept
{
    mwmp::Networking::get().getWorldController()->GetPacket(ID_CONTAINER)->Send(&scriptEvent, scriptEvent.guid);
}

void WorldFunctions::SetHour(unsigned short pid, double hour) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->hour = hour;
    player->month = -1;
    player->day = -1;

    mwmp::Networking::get().getPlayerController()->GetPacket(ID_GAME_TIME)->Send(player, false);
}

void WorldFunctions::SetMonth(unsigned short pid, int month) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->hour = -1;
    player->month = month;
    player->day = -1;

    mwmp::Networking::get().getPlayerController()->GetPacket(ID_GAME_TIME)->Send(player, false);

}

void WorldFunctions::SetDay(unsigned short pid, int day) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->hour = -1;
    player->month = -1;
    player->day = day;

    mwmp::Networking::get().getPlayerController()->GetPacket(ID_GAME_TIME)->Send(player, false);
}
