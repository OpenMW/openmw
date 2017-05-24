#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Log.hpp>
#include "PacketActorAttack.hpp"

using namespace mwmp;

PacketActorAttack::PacketActorAttack(RakNet::RakPeerInterface *peer) : ActorPacket(peer)
{
    packetID = ID_ACTOR_ATTACK;
}

void PacketActorAttack::Packet(RakNet::BitStream *bs, bool send)
{
    ActorPacket::Packet(bs, send);

    if (send)
        actorList->count = (unsigned int)(actorList->baseActors.size());
    else
        actorList->baseActors.clear();

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

        RW(actor.attack.target.refId, send);
        RW(actor.attack.target.refNumIndex, send);
        RW(actor.attack.target.mpNum, send);
        RW(actor.attack.target.guid, send);

        RW(actor.attack.spellId, send);
        RW(actor.attack.type, send);
        RW(actor.attack.success, send);
        RW(actor.attack.damage, send);

        RW(actor.attack.pressed, send);
        RW(actor.attack.knockdown, send);
        RW(actor.attack.block, send);
        
        if (!send)
        {
            actorList->baseActors.push_back(actor);
        }
    }
}
