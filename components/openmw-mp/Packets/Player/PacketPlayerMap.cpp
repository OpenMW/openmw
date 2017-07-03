#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketPlayerMap.hpp"

using namespace std;
using namespace mwmp;

PacketPlayerMap::PacketPlayerMap(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_MAP;
}

void PacketPlayerMap::Packet(RakNet::BitStream *bs, bool send)
{
    PlayerPacket::Packet(bs, send);

    if (send)
        player->mapChanges.count = (unsigned int)(player->mapChanges.cellsExplored.size());
    else
        player->mapChanges.cellsExplored.clear();

    RW(player->mapChanges.count, send);

    for (unsigned int i = 0; i < player->mapChanges.count; i++)
    {
        ESM::Cell cellExplored;

        if (send)
            cellExplored = player->mapChanges.cellsExplored.at(i);

        RW(cellExplored.mData, send, 1);
        RW(cellExplored.mName, send, 1);

        if (!send)
            player->mapChanges.cellsExplored.push_back(cellExplored);
    }
}
