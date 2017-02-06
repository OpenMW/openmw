#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketObjectRotate.hpp"

using namespace mwmp;

PacketObjectRotate::PacketObjectRotate(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_OBJECT_ROTATE;
}

void PacketObjectRotate::Packet(RakNet::BitStream *bs, BaseEvent *event, bool send)
{
    WorldPacket::Packet(bs, event, send);

    if (!send)
        event->objectChanges.objects.clear();
    else
        event->objectChanges.count = (unsigned int)(event->objectChanges.objects.size());

    RW(event->objectChanges.count, send);

    RW(event->cell.mData.mFlags, send);
    RW(event->cell.mData.mX, send);
    RW(event->cell.mData.mY, send);
    RW(event->cell.mName, send);

    WorldObject worldObject;

    for (unsigned int i = 0; i < event->objectChanges.count; i++)
    {
        if (send)
        {
            worldObject = event->objectChanges.objects.at(i);
        }

        RW(worldObject.refId, send);
        RW(worldObject.refNumIndex, send);
        RW(worldObject.pos.rot[0], send);
        RW(worldObject.pos.rot[1], send);
        RW(worldObject.pos.rot[2], send);

        if (!send)
        {
            event->objectChanges.objects.push_back(worldObject);
        }
    }
}
