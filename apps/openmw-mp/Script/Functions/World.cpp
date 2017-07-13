#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Base/BaseEvent.hpp>

#include <apps/openmw-mp/Networking.hpp>
#include <apps/openmw-mp/Player.hpp>
#include <apps/openmw-mp/Utils.hpp>
#include <apps/openmw-mp/Script/ScriptFunctions.hpp>

#include "World.hpp"

using namespace mwmp;

BaseEvent *readEvent;
BaseEvent writeEvent;

WorldObject tempWorldObject;
const WorldObject emptyWorldObject = {};

ContainerItem tempContainerItem;
const ContainerItem emptyContainerItem = {};

void WorldFunctions::ReadLastEvent() noexcept
{
    readEvent = mwmp::Networking::getPtr()->getLastEvent();
}

void WorldFunctions::InitializeEvent(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    writeEvent.cell.blank();
    writeEvent.worldObjects.clear();
    writeEvent.guid = player->guid;
}

unsigned int WorldFunctions::GetObjectChangesSize() noexcept
{
    return readEvent->worldObjectCount;
}

unsigned char WorldFunctions::GetEventAction() noexcept
{
    return readEvent->action;
}

const char *WorldFunctions::GetObjectRefId(unsigned int i) noexcept
{
    return readEvent->worldObjects.at(i).refId.c_str();
}

int WorldFunctions::GetObjectRefNumIndex(unsigned int i) noexcept
{
    return readEvent->worldObjects.at(i).refNumIndex;
}

int WorldFunctions::GetObjectMpNum(unsigned int i) noexcept
{
    return readEvent->worldObjects.at(i).mpNum;
}

int WorldFunctions::GetObjectCount(unsigned int i) noexcept
{
    return readEvent->worldObjects.at(i).count;
}

int WorldFunctions::GetObjectCharge(unsigned int i) noexcept
{
    return readEvent->worldObjects.at(i).charge;
}

int WorldFunctions::GetObjectGoldValue(unsigned int i) noexcept
{
    return readEvent->worldObjects.at(i).goldValue;
}

double WorldFunctions::GetObjectScale(unsigned int i) noexcept
{
    return readEvent->worldObjects.at(i).scale;
}

bool WorldFunctions::GetObjectState(unsigned int i) noexcept
{
    return readEvent->worldObjects.at(i).objectState;
}

int WorldFunctions::GetObjectDoorState(unsigned int i) noexcept
{
    return readEvent->worldObjects.at(i).doorState;
}

int WorldFunctions::GetObjectLockLevel(unsigned int i) noexcept
{
    return readEvent->worldObjects.at(i).lockLevel;
}

double WorldFunctions::GetObjectPosX(unsigned int i) noexcept
{
    return readEvent->worldObjects.at(i).position.pos[0];
}

double WorldFunctions::GetObjectPosY(unsigned int i) noexcept
{
    return readEvent->worldObjects.at(i).position.pos[1];
}

double WorldFunctions::GetObjectPosZ(unsigned int i) noexcept
{
    return readEvent->worldObjects.at(i).position.pos[2];
}

double WorldFunctions::GetObjectRotX(unsigned int i) noexcept
{
    return readEvent->worldObjects.at(i).position.rot[0];
}

double WorldFunctions::GetObjectRotY(unsigned int i) noexcept
{
    return readEvent->worldObjects.at(i).position.rot[1];
}

double WorldFunctions::GetObjectRotZ(unsigned int i) noexcept
{
    return readEvent->worldObjects.at(i).position.rot[2];
}

unsigned int WorldFunctions::GetContainerChangesSize(unsigned int objectIndex) noexcept
{
    return readEvent->worldObjects.at(objectIndex).containerItemCount;
}

const char *WorldFunctions::GetContainerItemRefId(unsigned int objectIndex, unsigned int itemIndex) noexcept
{
    return readEvent->worldObjects.at(objectIndex)
        .containerItems.at(itemIndex).refId.c_str();
}

int WorldFunctions::GetContainerItemCount(unsigned int objectIndex, unsigned int itemIndex) noexcept
{
    return readEvent->worldObjects.at(objectIndex)
        .containerItems.at(itemIndex).count;
}

int WorldFunctions::GetContainerItemCharge(unsigned int objectIndex, unsigned int itemIndex) noexcept
{
    return readEvent->worldObjects.at(objectIndex)
        .containerItems.at(itemIndex).charge;
}

int WorldFunctions::GetContainerItemActionCount(unsigned int objectIndex, unsigned int itemIndex) noexcept
{
    return readEvent->worldObjects.at(objectIndex)
        .containerItems.at(itemIndex).actionCount;
}

void WorldFunctions::SetEventCell(const char* cellDescription) noexcept
{
    writeEvent.cell = Utils::getCellFromDescription(cellDescription);
}

void WorldFunctions::SetEventAction(unsigned char action) noexcept
{
    writeEvent.action = action;
}

void WorldFunctions::SetObjectRefId(const char* refId) noexcept
{
    tempWorldObject.refId = refId;
}

void WorldFunctions::SetObjectRefNumIndex(int refNumIndex) noexcept
{
    tempWorldObject.refNumIndex = refNumIndex;
}

void WorldFunctions::SetObjectMpNum(int mpNum) noexcept
{
    tempWorldObject.mpNum = mpNum;
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

void WorldFunctions::SetObjectState(bool objectState) noexcept
{
    tempWorldObject.objectState = objectState;
}

void WorldFunctions::SetObjectDoorState(int doorState) noexcept
{
    tempWorldObject.doorState = doorState;
}

void WorldFunctions::SetObjectLockLevel(int lockLevel) noexcept
{
    tempWorldObject.lockLevel = lockLevel;
}

void WorldFunctions::SetObjectDisarmState(bool disarmState) noexcept
{
    tempWorldObject.isDisarmed = disarmState;
}

void WorldFunctions::SetObjectMasterState(bool masterState) noexcept
{
    tempWorldObject.hasMaster = masterState;
}

void WorldFunctions::SetObjectPosition(double x, double y, double z) noexcept
{
    tempWorldObject.position.pos[0] = x;
    tempWorldObject.position.pos[1] = y;
    tempWorldObject.position.pos[2] = z;
}

void WorldFunctions::SetObjectRotation(double x, double y, double z) noexcept
{
    tempWorldObject.position.rot[0] = x;
    tempWorldObject.position.rot[1] = y;
    tempWorldObject.position.rot[2] = z;
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
    writeEvent.worldObjects.push_back(tempWorldObject);

    tempWorldObject = emptyWorldObject;
}

void WorldFunctions::AddContainerItem() noexcept
{
    tempWorldObject.containerItems.push_back(tempContainerItem);

    tempContainerItem = emptyContainerItem;
}

void WorldFunctions::SendObjectPlace() noexcept
{
    mwmp::Networking::get().getWorldPacketController()->GetPacket(ID_OBJECT_PLACE)->setEvent(&writeEvent);
    mwmp::Networking::get().getWorldPacketController()->GetPacket(ID_OBJECT_PLACE)->Send(writeEvent.guid);
}

void WorldFunctions::SendObjectSpawn() noexcept
{
    mwmp::Networking::get().getWorldPacketController()->GetPacket(ID_OBJECT_SPAWN)->setEvent(&writeEvent);
    mwmp::Networking::get().getWorldPacketController()->GetPacket(ID_OBJECT_SPAWN)->Send(writeEvent.guid);
}

void WorldFunctions::SendObjectDelete() noexcept
{
    mwmp::Networking::get().getWorldPacketController()->GetPacket(ID_OBJECT_DELETE)->setEvent(&writeEvent);
    mwmp::Networking::get().getWorldPacketController()->GetPacket(ID_OBJECT_DELETE)->Send(writeEvent.guid);
}

void WorldFunctions::SendObjectLock() noexcept
{
    mwmp::Networking::get().getWorldPacketController()->GetPacket(ID_OBJECT_LOCK)->setEvent(&writeEvent);
    mwmp::Networking::get().getWorldPacketController()->GetPacket(ID_OBJECT_LOCK)->Send(writeEvent.guid);
}

void WorldFunctions::SendObjectTrap() noexcept
{
    mwmp::Networking::get().getWorldPacketController()->GetPacket(ID_OBJECT_TRAP)->setEvent(&writeEvent);
    mwmp::Networking::get().getWorldPacketController()->GetPacket(ID_OBJECT_TRAP)->Send(writeEvent.guid);
}

void WorldFunctions::SendObjectScale() noexcept
{
    mwmp::Networking::get().getWorldPacketController()->GetPacket(ID_OBJECT_SCALE)->setEvent(&writeEvent);
    mwmp::Networking::get().getWorldPacketController()->GetPacket(ID_OBJECT_SCALE)->Send(writeEvent.guid);
}

void WorldFunctions::SendObjectState() noexcept
{
    mwmp::Networking::get().getWorldPacketController()->GetPacket(ID_OBJECT_STATE)->setEvent(&writeEvent);
    mwmp::Networking::get().getWorldPacketController()->GetPacket(ID_OBJECT_STATE)->Send(writeEvent.guid);
}

void WorldFunctions::SendDoorState() noexcept
{
    mwmp::Networking::get().getWorldPacketController()->GetPacket(ID_DOOR_STATE)->setEvent(&writeEvent);
    mwmp::Networking::get().getWorldPacketController()->GetPacket(ID_DOOR_STATE)->Send(writeEvent.guid);
}

void WorldFunctions::SendContainer() noexcept
{
    mwmp::Networking::get().getWorldPacketController()->GetPacket(ID_CONTAINER)->setEvent(&writeEvent);
    mwmp::Networking::get().getWorldPacketController()->GetPacket(ID_CONTAINER)->Send(writeEvent.guid);
}

void WorldFunctions::SetHour(unsigned short pid, double hour) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->hour = hour;
    player->month = -1;
    player->day = -1;

    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_GAME_TIME)->setPlayer(player);
    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_GAME_TIME)->Send(false);
}

void WorldFunctions::SetMonth(unsigned short pid, int month) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->hour = -1;
    player->month = month;
    player->day = -1;

    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_GAME_TIME)->setPlayer(player);
    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_GAME_TIME)->Send(false);

}

void WorldFunctions::SetDay(unsigned short pid, int day) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->hour = -1;
    player->month = -1;
    player->day = day;

    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_GAME_TIME)->setPlayer(player);
    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_GAME_TIME)->Send(false);
}
