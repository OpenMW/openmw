//
// Created by koncord on 06.03.16.
//

#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketChatMessage.hpp"

mwmp::PacketChatMessage::PacketChatMessage(RakNet::RakPeerInterface *peer) : BasePacket(peer)
{
    packetID = ID_CHAT_MESSAGE;
}

void mwmp::PacketChatMessage::Packet(RakNet::BitStream *bs, mwmp::BasePlayer *player, bool send)
{
    BasePacket::Packet(bs, player, send);

    RW(*player->ChatMessage(), send);
}
