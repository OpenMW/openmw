//
// Created by koncord on 23.07.16.
//

#include "PacketGUIBoxes.hpp"
#include <components/openmw-mp/NetworkMessages.hpp>

using namespace mwmp;

PacketGUIBoxes::PacketGUIBoxes(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_GUI_MESSAGEBOX;
}

void PacketGUIBoxes::Packet(RakNet::BitStream *bs, BasePlayer *player, bool send)
{
    PlayerPacket::Packet(bs, player, send);

    RW(player->guiMessageBox.id, send);
    RW(player->guiMessageBox.type, send);
    RW(player->guiMessageBox.label, send);

    RW(player->guiMessageBox.data, send);

    if (player->guiMessageBox.type == BasePlayer::GUIMessageBox::CustomMessageBox)
        RW(player->guiMessageBox.buttons, send);
}

