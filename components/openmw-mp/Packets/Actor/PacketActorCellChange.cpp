#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Log.hpp>
#include "PacketActorCellChange.hpp"

using namespace mwmp;

PacketActorCellChange::PacketActorCellChange(RakNet::RakPeerInterface *peer) : ActorPacket(peer)
{
    packetID = ID_ACTOR_CELL_CHANGE;
}

void PacketActorCellChange::Actor(BaseActor &actor, bool send)
{
    RW(actor.cell.mData, send, 1);
    RW(actor.cell.mName, send, 1);

    RW(actor.position, send, 1);
    RW(actor.direction, send, 1);
}
