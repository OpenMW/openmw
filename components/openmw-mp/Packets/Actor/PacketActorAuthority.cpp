#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketActorAuthority.hpp"

using namespace mwmp;

PacketActorAuthority::PacketActorAuthority(RakNet::RakPeerInterface *peer) : ActorPacket(peer)
{
    packetID = ID_ACTOR_AUTHORITY;
}

void PacketActorAuthority::Packet(RakNet::BitStream *bs, bool send)
{
    ActorPacket::Packet(bs, send);

    if (!send)
        actorList->baseActors.clear();
    else
        actorList->count = (unsigned int)(actorList->baseActors.size());

    RW(actorList->count, send);

    RW(actorList->cell.mData.mFlags, send);
    RW(actorList->cell.mData.mX, send);
    RW(actorList->cell.mData.mY, send);
    RW(actorList->cell.mName, send);
}
