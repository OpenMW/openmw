#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>

#include "../Packets/Player/PacketClass.hpp"
#include "../Packets/Player/PacketPosition.hpp"
#include "../Packets/Player/PacketBaseInfo.hpp"
#include "../Packets/Player/PacketEquipment.hpp"
#include "../Packets/Player/PacketAttack.hpp"
#include "../Packets/Player/PacketDynamicStats.hpp"
#include "../Packets/Player/PacketResurrect.hpp"
#include "../Packets/Player/PacketDie.hpp"
#include "../Packets/Player/PacketCell.hpp"
#include "../Packets/Player/PacketSendMyID.hpp"
#include "../Packets/Player/PacketDisconnect.hpp"
#include "../Packets/Player/PacketDrawState.hpp"
#include "../Packets/Player/PacketChatMessage.hpp"
#include "../Packets/Player/PacketCharGen.hpp"
#include "../Packets/Player/PacketAttribute.hpp"
#include "../Packets/Player/PacketSkill.hpp"
#include "../Packets/Player/PacketLevel.hpp"
#include "../Packets/Player/PacketHandshake.hpp"
#include "../Packets/Player/PacketGUIBoxes.hpp"
#include "../Packets/Player/PacketTime.hpp"
#include "../Packets/Player/PacketLoaded.hpp"
#include "../Packets/Player/PacketInventory.hpp"
#include "../Packets/Player/PacketSpellbook.hpp"
#include "../Packets/Player/PacketConsole.hpp"

#include "PlayerPacketController.hpp"

template <typename T>
inline void AddPacket(mwmp::PlayerPacketController::packets_t *packets, RakNet::RakPeerInterface *peer)
{
    T *packet = new T(peer);
    typedef mwmp::PlayerPacketController::packets_t::value_type value_t;
    packets->insert(value_t(packet->GetPacketID(), value_t::second_type(packet)));
}

mwmp::PlayerPacketController::PlayerPacketController(RakNet::RakPeerInterface *peer)
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
    AddPacket<PacketInventory>(&packets, peer);
    AddPacket<PacketSpellbook>(&packets, peer);

    AddPacket<PacketConsole>(&packets, peer);
}


mwmp::PlayerPacket *mwmp::PlayerPacketController::GetPacket(RakNet::MessageID id)
{
    return packets[(unsigned char)id].get();
}

void mwmp::PlayerPacketController::SetStream(RakNet::BitStream *inStream, RakNet::BitStream *outStream)
{
    BOOST_FOREACH( packets_t::value_type &packet, packets )
        packet.second->SetStreams(inStream, outStream);
}

bool mwmp::PlayerPacketController::ContainsPacket(RakNet::MessageID id)
{
    BOOST_FOREACH(packets_t::value_type &packet, packets) {
        if (packet.first == id)
            return true;
    }
    return false;
}
