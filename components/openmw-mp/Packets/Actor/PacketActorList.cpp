#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketActorList.hpp"

using namespace mwmp;

PacketActorList::PacketActorList(RakNet::RakPeerInterface *peer) : ActorPacket(peer)
{
    packetID = ID_ACTOR_LIST;
}

void PacketActorList::Packet(RakNet::BitStream *bs, bool send)
{
    ActorPacket::Packet(bs, send);

    RW(actorList->action, send);

    if (send)
    {
        actorList->count = (unsigned int)(actorList->baseActors.size());
    }
    else
    {
        actorList->baseActors.clear();
    }

    RW(actorList->count, send);

    if (actorList->count > 2000)
    {
        actorList->isValid = false;
        return;
    }

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

        if (actor.refId.empty() || (actor.refNumIndex != 0 && actor.mpNum != 0))
        {
            actorList->isValid = false;
            return;
        }

        if (!send)
        {
            actorList->baseActors.push_back(actor);
        }
    }
}
