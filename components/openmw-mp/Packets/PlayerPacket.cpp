#include <components/openmw-mp/NetworkMessages.hpp>
#include <PacketPriority.h>
#include <RakPeer.h>
#include "PlayerPacket.hpp"

using namespace mwmp;

void PlayerPacket::Packet(RakNet::BitStream *bs, BasePlayer *player, bool send)
{
    this->player = player;
    this->bs = bs;

    if (send)
    {
        bs->Write(packetID);
        bs->Write(player->guid);
    }
}

PlayerPacket::PlayerPacket(RakNet::RakPeerInterface *peer)
{
    packetID = 0;
    priority = HIGH_PRIORITY;
    reliability = RELIABLE_ORDERED;
    this->peer = peer;
}

PlayerPacket::~PlayerPacket()
{

}

void PlayerPacket::Send(BasePlayer *player, RakNet::AddressOrGUID destination)
{
    bsSend->ResetWritePointer();
    Packet(bsSend, player, true);
    peer->Send(bsSend, priority, reliability, 0, destination, false);
}

void PlayerPacket::Send(BasePlayer *player, bool toOther)
{
    bsSend->ResetWritePointer();
    Packet(bsSend, player, true);
    peer->Send(bsSend, priority, reliability, 0, player->guid, toOther);
}

void PlayerPacket::Read(BasePlayer *player)
{
    Packet(bsRead, player, false);
}

void PlayerPacket::SetReadStream(RakNet::BitStream *bitStream)
{
    bsRead = bitStream;
}

void PlayerPacket::SetSendStream(RakNet::BitStream *bitStream)
{
    bsSend = bitStream;
}

void PlayerPacket::RequestData(RakNet::RakNetGUID player)
{
    bsSend->ResetWritePointer();
    bsSend->Write(packetID);
    bsSend->Write(player);
    peer->Send(bsSend, HIGH_PRIORITY, RELIABLE_ORDERED, 0, player, false);
}

void PlayerPacket::SetStreams(RakNet::BitStream *inStream, RakNet::BitStream *outStream)
{
    if (inStream != 0)
        bsRead = inStream;
    if (outStream != 0)
        bsSend = outStream;
}