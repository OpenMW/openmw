#include <components/openmw-mp/NetworkMessages.hpp>
#include <PacketPriority.h>
#include <RakPeer.h>
#include "WorldPacket.hpp"

using namespace mwmp;

void WorldPacket::Packet(RakNet::BitStream *bs, WorldEvent *event, bool send)
{
    this->event = event;
    this->bs = bs;

    if (send)
    {
        bs->Write(packetID);
        bs->Write(event->guid);
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

void WorldPacket::Send(WorldEvent *event, RakNet::AddressOrGUID destination)
{
    bsSend->ResetWritePointer();
    Packet(bsSend, event, true);
    peer->Send(bsSend, priority, reliability, 0, destination, false);
}

void WorldPacket::Send(WorldEvent *event, bool toOther)
{
    bsSend->ResetWritePointer();
    Packet(bsSend, event, true);
    peer->Send(bsSend, priority, reliability, 0, event->guid, toOther);
}

void WorldPacket::Read(WorldEvent *event)
{
    Packet(bsRead, event, false);
}

void WorldPacket::RequestData(RakNet::RakNetGUID guid)
{
    bsSend->ResetWritePointer();
    bsSend->Write(packetID);
    bsSend->Write(guid);
    peer->Send(bsSend, HIGH_PRIORITY, RELIABLE_ORDERED, 0, guid, false);
}
