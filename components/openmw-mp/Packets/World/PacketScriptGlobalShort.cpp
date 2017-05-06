#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketScriptGlobalShort.hpp"

using namespace mwmp;

PacketScriptGlobalShort::PacketScriptGlobalShort(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_SCRIPT_GLOBAL_SHORT;
}

void PacketScriptGlobalShort::Packet(RakNet::BitStream *bs, bool send)
{
    WorldPacket::Packet(bs, send);

    if (!send)
        event->worldObjects.clear();
    else
        event->worldObjectCount = (unsigned int)(event->worldObjects.size());

    RW(event->worldObjectCount, send);

    WorldObject worldObject;

    for (unsigned int i = 0; i < event->worldObjectCount; i++)
    {
        if (send)
        {
            worldObject = event->worldObjects.at(i);
        }

        RW(worldObject.varName, send);
        RW(worldObject.shortVal, send);

        if (!send)
        {
            event->worldObjects.push_back(worldObject);
        }
    }
}
