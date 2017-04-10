#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Log.hpp>
#include "PacketActorTest.hpp"

using namespace mwmp;

PacketActorTest::PacketActorTest(RakNet::RakPeerInterface *peer) : ActorPacket(peer)
{
    packetID = ID_ACTOR_TEST;
}

void PacketActorTest::Packet(RakNet::BitStream *bs, bool send)
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
        RW(actor.position, send);
        RW(actor.drawState, send);

        RW(actor.headPitch, send);
        RW(actor.headYaw, send);

        RW(actor.hasAnimation, send);
        RW(actor.hasAnimStates, send);
        RW(actor.hasMovement, send);

        if (actor.hasAnimation)
        {
            RW(actor.animation.groupname, send);
            RW(actor.animation.mode, send);
            RW(actor.animation.count, send);
            RW(actor.animation.persist, send);
        }

        if (actor.hasAnimStates)
        {
            RW(actor.animStates.idlestate, send);
            RW(actor.animStates.movestate, send);
            RW(actor.animStates.jumpstate, send);
            RW(actor.animStates.forcestateupdate, send);
        }

        if (actor.hasMovement)
        {
            RW(actor.movement.x, send);
            RW(actor.movement.y, send);
            RW(actor.movement.y, send);
        }

        if (!send)
        {
            actorList->baseActors.push_back(actor);
        }
    }
}
