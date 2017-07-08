#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketPlayerFaction.hpp"

using namespace std;
using namespace mwmp;

PacketPlayerFaction::PacketPlayerFaction(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_FACTION;
}

void PacketPlayerFaction::Packet(RakNet::BitStream *bs, bool send)
{
    PlayerPacket::Packet(bs, send);

    RW(player->factionChanges.action, send);

    if (send)
        player->factionChanges.count = (unsigned int)(player->factionChanges.factions.size());
    else
        player->factionChanges.factions.clear();

    RW(player->factionChanges.count, send);

    for (unsigned int i = 0; i < player->factionChanges.count; i++)
    {
        Faction faction;

        if (send)
            faction = player->factionChanges.factions.at(i);

        RW(faction.factionId, send, 1);

        if (player->factionChanges.action == FactionChanges::RANK)
            RW(faction.rank, send);

        if (player->factionChanges.action == FactionChanges::EXPULSION)
            RW(faction.isExpelled, send);

        if (!send)
            player->factionChanges.factions.push_back(faction);
    }
}
