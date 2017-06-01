#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Log.hpp>
#include "PacketActorPosition.hpp"

using namespace mwmp;

PacketActorPosition::PacketActorPosition(RakNet::RakPeerInterface *peer) : ActorPacket(peer)
{
    packetID = ID_ACTOR_POSITION;
}

void PacketActorPosition::Packet(RakNet::BitStream *bs, bool send)
{
    ActorPacket::Packet(bs, send);

    if (send)
        actorList->count = (unsigned int)(actorList->baseActors.size());
    else
        actorList->baseActors.clear();

    RW(actorList->count, send);

    if (actorList->count > maxActors)
    {
        actorList->isValid = false;
        return;
    }

    BaseActor actor;

    for (unsigned int i = 0; i < actorList->count; i++)
    {
        if (send)
            actor = actorList->baseActors.at(i);

        RW(actor.refNumIndex, send);
        RW(actor.mpNum, send);

        RW(actor.position, send, 1);
        RW(actor.direction, send, 1);

        actor.hasPositionData = true;

        if (!send)
            actorList->baseActors.push_back(actor);
    }
}
