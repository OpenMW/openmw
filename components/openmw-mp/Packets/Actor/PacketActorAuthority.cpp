#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketActorAuthority.hpp"

using namespace mwmp;

PacketActorAuthority::PacketActorAuthority(RakNet::RakPeerInterface *peer) : ActorPacket(peer)
{
    packetID = ID_ACTOR_AUTHORITY;
}

void PacketActorAuthority::Packet(RakNet::BitStream *bs, bool send)
{
    BasePacket::Packet(bs, send);

    RW(actorList->cell.mData, send, 1);
    RW(actorList->cell.mName, send, 1);
}
