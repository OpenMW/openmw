#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketObjectAnimPlay.hpp"

using namespace mwmp;

PacketObjectAnimPlay::PacketObjectAnimPlay(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_OBJECT_ANIM_PLAY;
}

void PacketObjectAnimPlay::Packet(RakNet::BitStream *bs, bool send)
{
    WorldPacket::Packet(bs, send);

    if (send)
        event->worldObjectCount = (unsigned int)(event->worldObjects.size());
    else
        event->worldObjects.clear();

    RW(event->worldObjectCount, send);

    if (event->worldObjectCount > maxObjects)
    {
        event->isValid = false;
        return;
    }

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
        RW(worldObject.animGroup, send);
        RW(worldObject.animMode, send);

        if (!send)
        {
            event->worldObjects.push_back(worldObject);
        }
    }
}
