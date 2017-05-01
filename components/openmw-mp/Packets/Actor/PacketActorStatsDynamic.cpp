#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Log.hpp>
#include <components/esm/creaturestats.hpp>
#include "PacketActorStatsDynamic.hpp"

using namespace mwmp;

PacketActorStatsDynamic::PacketActorStatsDynamic(RakNet::RakPeerInterface *peer) : ActorPacket(peer)
{
    packetID = ID_ACTOR_STATS_DYNAMIC;
}

void PacketActorStatsDynamic::Packet(RakNet::BitStream *bs, bool send)
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

    BaseActor actor;

    for (unsigned int i = 0; i < actorList->count; i++)
    {
        if (send)
        {
            actor = actorList->baseActors.at(i);
        }

        RW(actor.refId, send);
        RW(actor.refNumIndex, send);
        RW(actor.mpNum, send);
        
        RW(actor.creatureStats.mDynamic[0], send); // health
        RW(actor.creatureStats.mDynamic[1], send); // magic
        RW(actor.creatureStats.mDynamic[2], send); // fatigue

        actor.hasStatsDynamicData = true;

        if (!send)
        {
            actorList->baseActors.push_back(actor);
        }
    }
}
