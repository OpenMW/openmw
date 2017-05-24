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

    if (send)
        event->worldObjectCount = (unsigned int)(event->worldObjects.size());
    else
        event->worldObjects.clear();

    RW(event->worldObjectCount, send);

    RW(event->cell.mData.mFlags, send);
    RW(event->cell.mData.mX, send);
    RW(event->cell.mData.mY, send);
    RW(event->cell.mName, send);

    WorldObject worldObject;

    for (unsigned int i = 0; i < event->worldObjectCount; i++)
    {
        if (send)
        {
            worldObject = event->worldObjects.at(i);
        }

        RW(worldObject.refId, send);
        RW(worldObject.refNumIndex, send);
        RW(worldObject.mpNum, send);
        RW(worldObject.doorState, send);

        if (!send)
        {
            event->worldObjects.push_back(worldObject);
        }
    }
}
