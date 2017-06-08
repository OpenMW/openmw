#include <components/openmw-mp/NetworkMessages.hpp>
#include <PacketPriority.h>
#include <RakPeer.h>
#include "ActorPacket.hpp"

using namespace mwmp;

ActorPacket::ActorPacket(RakNet::RakPeerInterface *peer) : BasePacket(peer)
{
    packetID = 0;
    priority = HIGH_PRIORITY;
    reliability = RELIABLE_ORDERED;
    orderChannel = CHANNEL_ACTOR;
    this->peer = peer;
}

ActorPacket::~ActorPacket()
{

}

void ActorPacket::setActorList(BaseActorList *actorList)
{
    this->actorList = actorList;
    guid = actorList->guid;
}

void ActorPacket::Packet(RakNet::BitStream *bs, bool send)
{
    if(!PacketHeader(bs, send))
        return;

    BaseActor actor;

    for (unsigned int i = 0; i < actorList->count; i++)
    {
        if (send)
            actor = actorList->baseActors[i];

        RW(actor.refNumIndex, send);
        RW(actor.mpNum, send);

        Actor(actor, send);

        if (!send)
            actorList->baseActors.push_back(actor);
    }
}

bool ActorPacket::PacketHeader(RakNet::BitStream *bs, bool send)
{
    BasePacket::Packet(bs, send);

    RW(actorList->cell.mData, send, 1);
    RW(actorList->cell.mName, send, 1);

    if (send)
        actorList->count = (unsigned int)(actorList->baseActors.size());
    else
        actorList->baseActors.clear();

    RW(actorList->count, send);

    if (actorList->count > maxActors)
    {
        actorList->isValid = false;
        return false;
    }

    return true;
}


void ActorPacket::Actor(BaseActor &actor, bool send)
{

}
