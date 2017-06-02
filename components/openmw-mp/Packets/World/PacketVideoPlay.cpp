#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketVideoPlay.hpp"

using namespace mwmp;

PacketVideoPlay::PacketVideoPlay(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_VIDEO_PLAY;
}

void PacketVideoPlay::Object(WorldObject &worldObject, bool send)
{
    RW(worldObject.filename, send);
    RW(worldObject.allowSkipping, send);
}
