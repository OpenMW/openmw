//
// Created by koncord on 05.01.16.
//

#include <components/openmw-mp/NetworkMessages.hpp>
#include <PacketPriority.h>
#include <RakPeer.h>
#include "BasePacket.hpp"

using namespace mwmp;

void BasePacket::Packet(RakNet::BitStream *bs, BasePlayer *player, bool send)
{
    this->player = player;
    this->bs = bs;

    if(send)
    {
        bs->Write((RakNet::MessageID) packetID);
        bs->Write(player->guid);
    }
}

BasePacket::BasePacket(RakNet::RakPeerInterface *peer)
{
    packetID = 0;
    priority = HIGH_PRIORITY;
    reliability = RELIABLE_ORDERED;
    this->peer = peer;
}

BasePacket::~BasePacket()
{

}

void BasePacket::Send(BasePlayer *player, RakNet::AddressOrGUID destination)
{
    bsSend->ResetWritePointer();
    Packet(bsSend, player, true);
    peer->Send(bsSend, priority, reliability, 0, destination, false);
}

void BasePacket::Send(BasePlayer *player, bool toOther)
{
    bsSend->ResetWritePointer();
    Packet(bsSend, player, true);
    peer->Send(bsSend, priority, reliability, 0, player->guid, toOther);
}

void BasePacket::Read(BasePlayer *player)
{
    Packet(bsRead, player, false);
}

void BasePacket::SetReadStream(RakNet::BitStream *bitStream)
{
    bsRead = bitStream;
}

void BasePacket::SetSendStream(RakNet::BitStream *bitStream)
{
    bsSend = bitStream;
}

void BasePacket::RequestData(RakNet::RakNetGUID player)
{
    bsSend->ResetWritePointer();
    bsSend->Write((RakNet::MessageID) packetID);
    bsSend->Write(player);
    peer->Send(bsSend, HIGH_PRIORITY, RELIABLE_ORDERED, 0, player, false);
}

void BasePacket::SetStreams(RakNet::BitStream *inStream, RakNet::BitStream *outStream)
{
    if(inStream != 0)
        bsRead = inStream;
    if(outStream != 0)
        bsSend = outStream;
}