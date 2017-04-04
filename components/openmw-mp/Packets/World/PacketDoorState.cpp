#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketDoorState.hpp"

using namespace mwmp;

PacketDoorState::PacketDoorState(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_DOOR_STATE;
}

void PacketDoorState::Packet(RakNet::BitStream *bs, bool send)
{
    WorldPacket::Packet(bs, send);

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
        RW(worldObject.mpNum, send);
        RW(worldObject.doorState, send);

        if (!send)
        {
            event->objectChanges.objects.push_back(worldObject);
        }
    }
}
