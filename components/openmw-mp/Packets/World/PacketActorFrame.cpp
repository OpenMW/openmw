#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Log.hpp>
#include "PacketActorFrame.hpp"

using namespace mwmp;

PacketActorFrame::PacketActorFrame(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_ACTOR_FRAME;
}

void PacketActorFrame::Packet(RakNet::BitStream *bs, bool send)
{
    WorldPacket::Packet(bs, send);

    if (!send)
        event->objectChanges.objects.clear();
    else
        event->objectChanges.count = (unsigned int)(event->objectChanges.objects.size());

    RW(event->objectChanges.count, send);

    RW(event->cell.mData.mFlags, send);
    RW(event->cell.mData.mX, send);
    RW(event->cell.mData.mY, send);
    RW(event->cell.mName, send);

    WorldObject worldObject;

    for (unsigned int i = 0; i < event->objectChanges.count; i++)
    {
        if (send)
        {
            worldObject = event->objectChanges.objects.at(i);
        }

        RW(worldObject.refId, send);
        RW(worldObject.refNumIndex, send);
        RW(worldObject.mpNum, send);
        RW(worldObject.pos, send);
        RW(worldObject.direction, send);
        RW(worldObject.movementFlags, send);
        RW(worldObject.drawState, send);

        RW(worldObject.headPitch, send);
        RW(worldObject.headYaw, send);

        RW(worldObject.hasAnimation, send);
        RW(worldObject.hasAnimStates, send);
        RW(worldObject.hasMovement, send);

        if (worldObject.hasAnimation)
        {
            RW(worldObject.animation.groupname, send);
            RW(worldObject.animation.mode, send);
            RW(worldObject.animation.count, send);
            RW(worldObject.animation.persist, send);
        }

        if (worldObject.hasAnimStates)
        {
            RW(worldObject.animStates.idlestate, send);
            RW(worldObject.animStates.movestate, send);
            RW(worldObject.animStates.jumpstate, send);
            RW(worldObject.animStates.forcestateupdate, send);
        }

        if (worldObject.hasMovement)
        {
            RW(worldObject.movement.x, send);
            RW(worldObject.movement.y, send);
            RW(worldObject.movement.y, send);
        }

        if (!send)
        {
            event->objectChanges.objects.push_back(worldObject);
        }
    }
}
