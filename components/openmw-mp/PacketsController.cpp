//
// Created by koncord on 15.01.16.
//

#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>

#include "Packets/PacketClass.hpp"
#include "Packets/PacketPosition.hpp"
#include "Packets/PacketBaseInfo.hpp"
#include "components/openmw-mp/Packets/PacketEquipment.hpp"
#include "Packets/PacketAttack.hpp"
#include "Packets/PacketDynamicStats.hpp"
#include "Packets/PacketResurrect.hpp"
#include "Packets/PacketDie.hpp"
#include "Packets/PacketCell.hpp"
#include "Packets/PacketSendMyID.hpp"
#include "Packets/PacketDisconnect.hpp"
#include "Packets/PacketDrawState.hpp"
#include "Packets/PacketChatMessage.hpp"
#include "Packets/PacketCharGen.hpp"
#include "Packets/PacketAttribute.hpp"
#include "Packets/PacketSkill.hpp"
#include "Packets/PacketLevel.hpp"
#include "Packets/PacketHandshake.hpp"
#include "Packets/PacketGUIBoxes.hpp"
#include "Packets/PacketTime.hpp"
#include "Packets/PacketLoaded.hpp"

#include "PacketsController.hpp"

template <typename T>
inline void AddPacket(mwmp::PacketsController::packets_t *packets, RakNet::RakPeerInterface *peer)
{
    T *packet = new T(peer);
    typedef mwmp::PacketsController::packets_t::value_type value_t;
    packets->insert(value_t(packet->GetPacketID(), value_t::second_type(packet)));
}

mwmp::PacketsController::PacketsController(RakNet::RakPeerInterface *peer)
{
    AddPacket<PacketPosition>(&packets, peer);
    AddPacket<PacketCell>(&packets, peer);
    AddPacket<PacketBaseInfo>(&packets, peer);
    AddPacket<PacketEquipment>(&packets, peer);

    AddPacket<PacketAttack>(&packets, peer);
    AddPacket<PacketDynamicStats>(&packets, peer);
    AddPacket<PacketResurrect>(&packets, peer);

    AddPacket<PacketDie>(&packets, peer);
    AddPacket<PacketDrawState>(&packets, peer);
    AddPacket<PacketSendMyID>(&packets, peer);
    AddPacket<PacketDisconnect>(&packets, peer);

    AddPacket<PacketChatMessage>(&packets, peer);
    AddPacket<PacketCharGen>(&packets, peer);
    AddPacket<PacketAttribute>(&packets, peer);
    AddPacket<PacketSkill>(&packets, peer);
    AddPacket<PacketLevel>(&packets, peer);

    AddPacket<PacketHandshake>(&packets, peer);
    AddPacket<PacketGUIBoxes>(&packets, peer);
    AddPacket<PacketClass>(&packets, peer);
    AddPacket<PacketTime>(&packets, peer);
    AddPacket<PacketLoaded>(&packets, peer);
}


mwmp::BasePacket *mwmp::PacketsController::GetPacket(RakNet::MessageID id)
{
    return packets[(unsigned char)id].get();
}

void mwmp::PacketsController::SetStream(RakNet::BitStream *inStream, RakNet::BitStream *outStream)
{
    BOOST_FOREACH( packets_t::value_type &packet, packets )
        packet.second->SetStreams(inStream, outStream);
}