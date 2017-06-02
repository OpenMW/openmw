#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketObjectDelete.hpp"

using namespace mwmp;

PacketObjectDelete::PacketObjectDelete(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_OBJECT_DELETE;
    hasCellData = true;
}
