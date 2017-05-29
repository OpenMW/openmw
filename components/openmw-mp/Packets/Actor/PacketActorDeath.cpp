#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Log.hpp>
#include "PacketActorDeath.hpp"

using namespace mwmp;

PacketActorDeath::PacketActorDeath(RakNet::RakPeerInterface *peer) : ActorPacket(peer)
{
    packetID = ID_ACTOR_DEATH;
}

void PacketActorDeath::Packet(RakNet::BitStream *bs, bool send)
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
        
        // Placeholder to be filled in later

        if (!send)
        {
            actorList->baseActors.push_back(actor);
        }
    }
}
