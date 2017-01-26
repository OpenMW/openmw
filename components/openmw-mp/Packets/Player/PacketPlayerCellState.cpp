#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketPlayerCellState.hpp"


mwmp::PacketPlayerCellState::PacketPlayerCellState(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_CELL_STATE;
    priority = IMMEDIATE_PRIORITY;
    reliability = RELIABLE_ORDERED;
}

void mwmp::PacketPlayerCellState::Packet(RakNet::BitStream *bs, mwmp::BasePlayer *player, bool send)
{
    PlayerPacket::Packet(bs, player, send);

    RW(player->cellStateChanges.action, send);

    if (!send)
        player->cellStateChanges.cells.clear();
    else
        player->cellStateChanges.count = (unsigned int)(player->cellStateChanges.cells.size());

    RW(player->cellStateChanges.count, send);

    for (unsigned int i = 0; i < player->cellStateChanges.count; i++)
    {
        ESM::Cell cell;

        if (send)
        {
            cell = player->cellStateChanges.cells[i];
        }

        RW(cell.mData.mFlags, send);
        RW(cell.mData.mX, send);
        RW(cell.mData.mY, send);
        RW(cell.mName, send);

        if (!send)
        {
            player->cellStateChanges.cells.push_back(cell);
        }
    }
}
