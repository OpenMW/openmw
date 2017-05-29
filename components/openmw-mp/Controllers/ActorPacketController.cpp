#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>

#include "../Packets/Actor/PacketActorList.hpp"
#include "../Packets/Actor/PacketActorAuthority.hpp"
#include "../Packets/Actor/PacketActorTest.hpp"
#include "../Packets/Actor/PacketActorAI.hpp"
#include "../Packets/Actor/PacketActorAnimFlags.hpp"
#include "../Packets/Actor/PacketActorAnimPlay.hpp"
#include "../Packets/Actor/PacketActorAttack.hpp"
#include "../Packets/Actor/PacketActorCellChange.hpp"
#include "../Packets/Actor/PacketActorDeath.hpp"
#include "../Packets/Actor/PacketActorEquipment.hpp"
#include "../Packets/Actor/PacketActorPosition.hpp"
#include "../Packets/Actor/PacketActorStatsDynamic.hpp"
#include "../Packets/Actor/PacketActorSpeech.hpp"


#include "ActorPacketController.hpp"

template <typename T>
inline void AddPacket(mwmp::ActorPacketController::packets_t *packets, RakNet::RakPeerInterface *peer)
{
    T *packet = new T(peer);
    typedef mwmp::ActorPacketController::packets_t::value_type value_t;
    packets->insert(value_t(packet->GetPacketID(), value_t::second_type(packet)));
}

mwmp::ActorPacketController::ActorPacketController(RakNet::RakPeerInterface *peer)
{
    AddPacket<PacketActorList>(&packets, peer);
    AddPacket<PacketActorAuthority>(&packets, peer);
    AddPacket<PacketActorTest>(&packets, peer);
    AddPacket<PacketActorAI>(&packets, peer);
    AddPacket<PacketActorAnimFlags>(&packets, peer);
    AddPacket<PacketActorAnimPlay>(&packets, peer);
    AddPacket<PacketActorAttack>(&packets, peer);
    AddPacket<PacketActorCellChange>(&packets, peer);
    AddPacket<PacketActorDeath>(&packets, peer);
    AddPacket<PacketActorEquipment>(&packets, peer);
    AddPacket<PacketActorPosition>(&packets, peer);
    AddPacket<PacketActorSpeech>(&packets, peer);
    AddPacket<PacketActorStatsDynamic>(&packets, peer);
}


mwmp::ActorPacket *mwmp::ActorPacketController::GetPacket(RakNet::MessageID id)
{
    return packets[(unsigned char)id].get();
}

void mwmp::ActorPacketController::SetStream(RakNet::BitStream *inStream, RakNet::BitStream *outStream)
{
    BOOST_FOREACH( packets_t::value_type &packet, packets )
        packet.second->SetStreams(inStream, outStream);
}

bool mwmp::ActorPacketController::ContainsPacket(RakNet::MessageID id)
{
    BOOST_FOREACH(packets_t::value_type &packet, packets) {
        if (packet.first == id)
            return true;
    }
    return false;
}
