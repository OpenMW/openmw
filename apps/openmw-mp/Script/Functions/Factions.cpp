#include "Factions.hpp"
#include <apps/openmw-mp/Script/ScriptFunctions.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <apps/openmw-mp/Networking.hpp>
#include <components/misc/stringops.hpp>

using namespace mwmp;

unsigned int FactionFunctions::GetFactionChangesSize(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->factionChanges.count;
}

void FactionFunctions::AddFaction(unsigned short pid, const char* factionId, unsigned int rank, bool isExpelled) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    mwmp::Faction faction;
    faction.factionId = factionId;
    faction.rank = rank;
    faction.isExpelled = isExpelled;

    player->factionChangesBuffer.factions.push_back(faction);
}

const char *FactionFunctions::GetFactionId(unsigned short pid, unsigned int i) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, "");

    if (i >= player->factionChanges.count)
        return "invalid";

    return player->factionChanges.factions.at(i).factionId.c_str();
}

int FactionFunctions::GetFactionRank(unsigned short pid, unsigned int i) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->factionChanges.factions.at(i).rank;
}

int FactionFunctions::GetFactionExpelledState(unsigned short pid, unsigned int i) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->factionChanges.factions.at(i).isExpelled;
}

void FactionFunctions::SendFactionChanges(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    std::swap(player->factionChanges, player->factionChangesBuffer);
    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_FACTION)->setPlayer(player);
    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_FACTION)->Send(false);
    player->factionChanges = std::move(player->factionChangesBuffer);
    player->factionChangesBuffer.factions.clear();
}
