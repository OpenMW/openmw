#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Log.hpp>
#include "PacketActorAnimFlags.hpp"

using namespace mwmp;

PacketActorAnimFlags::PacketActorAnimFlags(RakNet::RakPeerInterface *peer) : ActorPacket(peer)
{
    packetID = ID_ACTOR_ANIM_FLAGS;
}

void PacketActorAnimFlags::Packet(RakNet::BitStream *bs, bool send)
{
    ActorPacket::Packet(bs, send);

    if (send)
        actorList->count = (unsigned int)(actorList->baseActors.size());
    else
        actorList->baseActors.clear();

    RW(actorList->count, send);

    BaseActor actor;

    for (unsigned int i = 0; i < actorList->count; i++)
    {
        if (send)
        {
            actor = actorList->baseActors.at(i);
        }

        RW(actor.refNumIndex, send);
        RW(actor.mpNum, send);

        RW(actor.movementFlags, send);
        RW(actor.drawState, send);
        RW(actor.isFlying, send);

        if (!send)
        {
            actorList->baseActors.push_back(actor);
        }
    }
    printf("Packet size: %d\n", bs->GetNumberOfBytesUsed());
}
