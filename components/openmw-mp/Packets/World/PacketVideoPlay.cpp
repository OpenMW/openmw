#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketVideoPlay.hpp"

using namespace mwmp;

PacketVideoPlay::PacketVideoPlay(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_VIDEO_PLAY;
}

void PacketVideoPlay::Packet(RakNet::BitStream *bs, WorldEvent *event, bool send)
{
    WorldPacket::Packet(bs, event, send);

    if (!send)
        event->objectChanges.objects.clear();
    else
        event->objectChanges.count = (unsigned int)(event->objectChanges.objects.size());

    RW(event->objectChanges.count, send);

    WorldObject worldObject;

    for (unsigned int i = 0; i < event->objectChanges.count; i++)
    {
        if (send)
        {
            worldObject = event->objectChanges.objects[i];
        }

        RW(worldObject.filename, send);
        RW(worldObject.allowSkipping, send);

        if (!send)
        {
            event->objectChanges.objects.push_back(worldObject);
        }
    }
}
