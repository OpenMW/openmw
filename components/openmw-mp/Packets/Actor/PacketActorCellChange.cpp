#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Log.hpp>
#include "PacketActorCellChange.hpp"

using namespace mwmp;

PacketActorCellChange::PacketActorCellChange(RakNet::RakPeerInterface *peer) : ActorPacket(peer)
{
    packetID = ID_ACTOR_CELL_CHANGE;
}

void PacketActorCellChange::Packet(RakNet::BitStream *bs, bool send)
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

        RW(actor.cell.mData, send, 1);
        RW(actor.cell.mName, send, 1);

        RW(actor.position, send, 1);
        RW(actor.direction, send, 1);

        if (!send)
            actorList->baseActors.push_back(actor);
    }
}
