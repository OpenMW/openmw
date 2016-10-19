#include <components/openmw-mp/NetworkMessages.hpp>
#include <PacketPriority.h>
#include <RakPeer.h>
#include "WorldPacket.hpp"

using namespace mwmp;

void WorldPacket::Packet(RakNet::BitStream *bs, BasePlayer *player, bool send)
{
    this->player = player;
    this->bs = bs;

    if (send)
    {
        bs->Write(packetID);
        bs->Write(player->guid);
    }
}

WorldPacket::WorldPacket(RakNet::RakPeerInterface *peer) : BasePacket(peer)
{
    packetID = 0;
    priority = HIGH_PRIORITY;
    reliability = RELIABLE_ORDERED;
    this->peer = peer;
}

WorldPacket::~WorldPacket()
{

}

void WorldPacket::Send(BasePlayer *player, RakNet::AddressOrGUID destination)
{
    bsSend->ResetWritePointer();
    Packet(bsSend, player, true);
    peer->Send(bsSend, priority, reliability, 0, destination, false);
}

void WorldPacket::Send(BasePlayer *player, bool toOther)
{
    bsSend->ResetWritePointer();
    Packet(bsSend, player, true);
    peer->Send(bsSend, priority, reliability, 0, player->guid, toOther);
}

void WorldPacket::Read(BasePlayer *player)
{
    Packet(bsRead, player, false);
}

void WorldPacket::RequestData(RakNet::RakNetGUID guid)
{
    bsSend->ResetWritePointer();
    bsSend->Write(packetID);
    bsSend->Write(guid);
    peer->Send(bsSend, HIGH_PRIORITY, RELIABLE_ORDERED, 0, guid, false);
}
