#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketVideoPlay.hpp"

using namespace mwmp;

PacketVideoPlay::PacketVideoPlay(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_VIDEO_PLAY;
}

void PacketVideoPlay::Packet(RakNet::BitStream *bs, bool send)
{
    WorldPacket::Packet(bs, send);

    if (send)
        event->worldObjectCount = (unsigned int)(event->worldObjects.size());
    else
        event->worldObjects.clear();

    RW(event->worldObjectCount, send);

    WorldObject worldObject;

    for (unsigned int i = 0; i < event->worldObjectCount; i++)
    {
        if (send)
        {
            worldObject = event->worldObjects.at(i);
        }

        RW(worldObject.filename, send);
        RW(worldObject.allowSkipping, send);

        if (!send)
        {
            event->worldObjects.push_back(worldObject);
        }
    }
}
