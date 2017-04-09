#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>

#include "../Packets/World/PacketObjectDelete.hpp"
#include "../Packets/World/PacketObjectPlace.hpp"
#include "../Packets/World/PacketObjectScale.hpp"
#include "../Packets/World/PacketObjectLock.hpp"
#include "../Packets/World/PacketObjectUnlock.hpp"
#include "../Packets/World/PacketObjectMove.hpp"
#include "../Packets/World/PacketObjectRotate.hpp"
#include "../Packets/World/PacketObjectAnimPlay.hpp"

#include "../Packets/World/PacketContainer.hpp"
#include "../Packets/World/PacketDoorState.hpp"
#include "../Packets/World/PacketMusicPlay.hpp"
#include "../Packets/World/PacketVideoPlay.hpp"

#include "../Packets/World/PacketScriptLocalShort.hpp"
#include "../Packets/World/PacketScriptLocalFloat.hpp"
#include "../Packets/World/PacketScriptMemberShort.hpp"
#include "../Packets/World/PacketScriptGlobalShort.hpp"

#include "WorldPacketController.hpp"

template <typename T>
inline void AddPacket(mwmp::WorldPacketController::packets_t *packets, RakNet::RakPeerInterface *peer)
{
    T *packet = new T(peer);
    typedef mwmp::WorldPacketController::packets_t::value_type value_t;
    packets->insert(value_t(packet->GetPacketID(), value_t::second_type(packet)));
}

mwmp::WorldPacketController::WorldPacketController(RakNet::RakPeerInterface *peer)
{
    AddPacket<PacketObjectDelete>(&packets, peer);
    AddPacket<PacketObjectPlace>(&packets, peer);
    AddPacket<PacketObjectScale>(&packets, peer);
    AddPacket<PacketObjectLock>(&packets, peer);
    AddPacket<PacketObjectUnlock>(&packets, peer);
    AddPacket<PacketObjectMove>(&packets, peer);
    AddPacket<PacketObjectRotate>(&packets, peer);
    AddPacket<PacketObjectAnimPlay>(&packets, peer);

    AddPacket<PacketContainer>(&packets, peer);
    AddPacket<PacketDoorState>(&packets, peer);
    AddPacket<PacketMusicPlay>(&packets, peer);
    AddPacket<PacketVideoPlay>(&packets, peer);

    AddPacket<PacketScriptLocalShort>(&packets, peer);
    AddPacket<PacketScriptLocalFloat>(&packets, peer);
    AddPacket<PacketScriptMemberShort>(&packets, peer);
    AddPacket<PacketScriptGlobalShort>(&packets, peer);
}


mwmp::WorldPacket *mwmp::WorldPacketController::GetPacket(RakNet::MessageID id)
{
    return packets[(unsigned char)id].get();
}

void mwmp::WorldPacketController::SetStream(RakNet::BitStream *inStream, RakNet::BitStream *outStream)
{
    BOOST_FOREACH( packets_t::value_type &packet, packets )
        packet.second->SetStreams(inStream, outStream);
}

bool mwmp::WorldPacketController::ContainsPacket(RakNet::MessageID id)
{
    BOOST_FOREACH(packets_t::value_type &packet, packets) {
        if (packet.first == id)
            return true;
    }
    return false;
}
