#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketScriptGlobalShort.hpp"

using namespace mwmp;

PacketScriptGlobalShort::PacketScriptGlobalShort(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_SCRIPT_GLOBAL_SHORT;
}

void PacketScriptGlobalShort::Packet(RakNet::BitStream *bs, BaseEvent *event, bool send)
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
            worldObject = event->objectChanges.objects.at(i);
        }

        RW(worldObject.varName, send);
        RW(worldObject.shortVal, send);

        if (!send)
        {
            event->objectChanges.objects.push_back(worldObject);
        }
    }
}
