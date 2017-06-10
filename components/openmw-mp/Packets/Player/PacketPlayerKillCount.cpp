#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketPlayerKillCount.hpp"

mwmp::PacketPlayerKillCount::PacketPlayerKillCount(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_KILL_COUNT;
}

void mwmp::PacketPlayerKillCount::Packet(RakNet::BitStream *bs, bool send)
{
    PlayerPacket::Packet(bs, send);

    if (send)
        player->killChanges.count = (unsigned int)(player->killChanges.kills.size());
    else
        player->killChanges.kills.clear();

    RW(player->killChanges.count, send);

    for (unsigned int i = 0; i < player->killChanges.count; i++)
    {
        Kill kill;

        if (send)
            kill = player->killChanges.kills.at(i);

        RW(kill.refId, send, 1);
        RW(kill.number, send);

        if (!send)
            player->killChanges.kills.push_back(kill);
    }
}
