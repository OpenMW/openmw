#include "Factions.hpp"
#include <apps/openmw-mp/Script/ScriptFunctions.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <apps/openmw-mp/Networking.hpp>
#include <components/misc/stringops.hpp>

using namespace mwmp;

void FactionFunctions::InitializeFactionChanges(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    player->factionChanges.factions.clear();
}

unsigned int FactionFunctions::GetFactionChangesSize(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->factionChanges.count;
}

unsigned char FactionFunctions::GetFactionChangesAction(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->factionChanges.action;
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

bool FactionFunctions::GetFactionExpelledState(unsigned short pid, unsigned int i) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->factionChanges.factions.at(i).isExpelled;
}

void FactionFunctions::SetFactionChangesAction(unsigned short pid, unsigned char action) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    player->factionChanges.action = action;
}

void FactionFunctions::AddFaction(unsigned short pid, const char* factionId, unsigned int rank, bool isExpelled) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    mwmp::Faction faction;
    faction.factionId = factionId;
    faction.rank = rank;
    faction.isExpelled = isExpelled;

    player->factionChanges.factions.push_back(faction);
}

void FactionFunctions::SendFactionChanges(unsigned short pid, bool toOthers) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_FACTION)->setPlayer(player);
    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_FACTION)->Send(toOthers);
}
