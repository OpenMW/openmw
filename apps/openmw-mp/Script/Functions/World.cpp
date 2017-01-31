#include <regex>

#include <apps/openmw-mp/Player.hpp>
#include <apps/openmw-mp/Script/ScriptFunctions.hpp>
#include <apps/openmw-mp/Networking.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Base/WorldEvent.hpp>
#include "World.hpp"

using namespace mwmp;

static WorldEvent *worldEvent = nullptr;
static WorldObject tempWorldObject;

std::regex exteriorCellPattern("^(-?\\d+), (-?\\d+)$");

void WorldFunctions::CreateWorldEvent(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    if (worldEvent)
    {
        delete worldEvent;
        worldEvent = nullptr;
    }

    worldEvent = new WorldEvent(player->guid);
}

void WorldFunctions::AddWorldObject() noexcept
{
    WorldObject worldObject;
    worldObject.refId = tempWorldObject.refId;
    worldObject.refNumIndex = tempWorldObject.refNumIndex;
    worldObject.count = tempWorldObject.count;
    worldObject.goldValue = tempWorldObject.goldValue;
    worldObject.pos = tempWorldObject.pos;

    worldEvent->objectChanges.objects.push_back(worldObject);
}

void WorldFunctions::SetWorldEventCell(const char* cellDescription) noexcept
{
    std::string description = cellDescription;
    std::smatch baseMatch;

    if (std::regex_match(description, baseMatch, exteriorCellPattern))
    {
        worldEvent->cell.mData.mFlags &= ~ESM::Cell::Interior;

        // The first sub match is the whole string, so check for a length of 3
        if (baseMatch.size() == 3)
        {
            worldEvent->cell.mData.mX = stoi(baseMatch[1].str());
            worldEvent->cell.mData.mY = stoi(baseMatch[2].str());
        }
    }
    else
    {
        worldEvent->cell.mData.mFlags |= ESM::Cell::Interior;
        worldEvent->cell.mName = description;
    }
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

void WorldFunctions::SetObjectGoldValue(int goldValue) noexcept
{
    tempWorldObject.goldValue = goldValue;
}

void WorldFunctions::SetObjectScale(int scale) noexcept
{
    tempWorldObject.scale = scale;
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

unsigned int WorldFunctions::GetObjectChangesSize() noexcept
{
    return mwmp::Networking::getPtr()->getLastEvent()->objectChanges.count;
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

int WorldFunctions::GetObjectGoldValue(unsigned int i) noexcept
{
    return mwmp::Networking::getPtr()->getLastEvent()->objectChanges.objects.at(i).goldValue;
}

int WorldFunctions::GetObjectScale(unsigned int i) noexcept
{
    return mwmp::Networking::getPtr()->getLastEvent()->objectChanges.objects.at(i).scale;
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

void WorldFunctions::SendObjectDelete() noexcept
{
    mwmp::Networking::get().getWorldController()->GetPacket(ID_OBJECT_DELETE)->Send(worldEvent, worldEvent->guid);
}

void WorldFunctions::SendObjectPlace() noexcept
{
    mwmp::Networking::get().getWorldController()->GetPacket(ID_OBJECT_PLACE)->Send(worldEvent, worldEvent->guid);
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
