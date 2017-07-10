#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketObjectState.hpp"

using namespace mwmp;

PacketObjectState::PacketObjectState(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_OBJECT_STATE;
    hasCellData = true;
}
