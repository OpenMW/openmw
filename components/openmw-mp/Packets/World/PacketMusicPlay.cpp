#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketMusicPlay.hpp"

using namespace mwmp;

PacketMusicPlay::PacketMusicPlay(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_MUSIC_PLAY;
}

void PacketMusicPlay::Object(WorldObject &worldObject, bool send)
{
    RW(worldObject.filename, send);
}
