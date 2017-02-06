#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketScriptLocalShort.hpp"

using namespace mwmp;

PacketScriptLocalShort::PacketScriptLocalShort(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_SCRIPT_LOCAL_SHORT;
}

void PacketScriptLocalShort::Packet(RakNet::BitStream *bs, BaseEvent *event, bool send)
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
        RW(worldObject.index, send);
        RW(worldObject.shortVal, send);

        if (!send)
        {
            event->objectChanges.objects.push_back(worldObject);
        }
    }
}
