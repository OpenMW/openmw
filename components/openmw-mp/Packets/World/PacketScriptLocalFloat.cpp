#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketScriptLocalFloat.hpp"

using namespace mwmp;

PacketScriptLocalFloat::PacketScriptLocalFloat(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_SCRIPT_LOCAL_FLOAT;
}

void PacketScriptLocalFloat::Packet(RakNet::BitStream *bs, bool send)
{
    WorldPacket::Packet(bs, send);

    if (!send)
        event->worldObjects.clear();
    else
        event->worldObjectCount = (unsigned int)(event->worldObjects.size());

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
        RW(worldObject.index, send);
        RW(worldObject.floatVal, send);

        if (!send)
        {
            event->worldObjects.push_back(worldObject);
        }
    }
}
