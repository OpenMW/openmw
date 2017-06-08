#include <components/openmw-mp/NetworkMessages.hpp>
#include <PacketPriority.h>
#include <RakPeer.h>
#include "WorldPacket.hpp"

using namespace mwmp;

WorldPacket::WorldPacket(RakNet::RakPeerInterface *peer) : BasePacket(peer)
{
    hasCellData = false;
    packetID = 0;
    priority = HIGH_PRIORITY;
    reliability = RELIABLE_ORDERED;
    orderChannel = CHANNEL_WORLDEVENT;
    this->peer = peer;
}

WorldPacket::~WorldPacket()
{

}

void WorldPacket::setEvent(BaseEvent *event)
{
    this->event = event;
    guid = event->guid;
}

void WorldPacket::Packet(RakNet::BitStream *bs, bool send)
{
    if(!PacketHeader(bs, send))
        return;

    WorldObject worldObject;
    for (unsigned int i = 0; i < event->worldObjectCount; i++)
    {
        if (send)
            worldObject = event->worldObjects.at(i);

        Object(worldObject, send);

        if (!send)
            event->worldObjects.push_back(worldObject);
    }
}

bool WorldPacket::PacketHeader(RakNet::BitStream *bs, bool send)
{
    BasePacket::Packet(bs, send);

    if (send)
        event->worldObjectCount = (unsigned int)(event->worldObjects.size());
    else
        event->worldObjects.clear();

    RW(event->worldObjectCount, send);

    if (event->worldObjectCount > maxObjects)
    {
        event->isValid = false;
        return false;
    }

    if(hasCellData)
    {
        RW(event->cell.mData, send, 1);
        RW(event->cell.mName, send, 1);
    }

    return true;
}

void WorldPacket::Object(WorldObject &worldObject, bool send)
{
    RW(worldObject.refId, send);
    RW(worldObject.refNumIndex, send);
    RW(worldObject.mpNum, send);
}
