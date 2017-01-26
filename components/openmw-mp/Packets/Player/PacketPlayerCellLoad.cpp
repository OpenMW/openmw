#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketPlayerCellLoad.hpp"


mwmp::PacketPlayerCellLoad::PacketPlayerCellLoad(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_CELL_CHANGE;
    priority = IMMEDIATE_PRIORITY;
    reliability = RELIABLE_ORDERED;
}

void mwmp::PacketPlayerCellLoad::Packet(RakNet::BitStream *bs, mwmp::BasePlayer *player, bool send)
{
    PlayerPacket::Packet(bs, player, send);

    RW(player->cellLoadChanges.action, send);

    if (!send)
        player->cellLoadChanges.cells.clear();
    else
        player->cellLoadChanges.count = (unsigned int)(player->cellLoadChanges.cells.size());

    RW(player->cellLoadChanges.count, send);

    for (unsigned int i = 0; i < player->cellLoadChanges.count; i++)
    {
        ESM::Cell cellLoaded;

        if (send)
        {
            cellLoaded = player->cellLoadChanges.cells[i];
        }

        RW(cellLoaded.mData.mFlags, send);
        RW(cellLoaded.mData.mX, send);
        RW(cellLoaded.mData.mY, send);
        RW(cellLoaded.mName, send);

        if (!send)
        {
            player->cellLoadChanges.cells.push_back(cellLoaded);
        }
    }
}
