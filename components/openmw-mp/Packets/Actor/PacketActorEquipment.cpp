#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Log.hpp>
#include "PacketActorEquipment.hpp"

using namespace mwmp;

PacketActorEquipment::PacketActorEquipment(RakNet::RakPeerInterface *peer) : ActorPacket(peer)
{
    packetID = ID_ACTOR_EQUIPMENT;
}

void PacketActorEquipment::Actor(BaseActor &actor, bool send)
{
    for (int j = 0; j < 19; j++)
    {
        RW(actor.equipedItems[j].refId, send);
        RW(actor.equipedItems[j].count, send);
        RW(actor.equipedItems[j].charge, send);
    }
}
