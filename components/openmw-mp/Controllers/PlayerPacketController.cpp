#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>

#include "../Packets/Player/PacketDisconnect.hpp"
#include "../Packets/Player/PacketChatMessage.hpp"
#include "../Packets/Player/PacketPlayerCharGen.hpp"
#include "../Packets/Player/PacketHandshake.hpp"
#include "../Packets/Player/PacketGUIBoxes.hpp"
#include "../Packets/Player/PacketLoaded.hpp"
#include "../Packets/Player/PacketConsole.hpp"
#include "../Packets/Player/PacketGameTime.hpp"
#include "../Packets/Player/PacketGameWeather.hpp"
#include "../Packets/Player/PacketPlayerActiveSkills.hpp"
#include "../Packets/Player/PacketPlayerBaseInfo.hpp"
#include "../Packets/Player/PacketPlayerAnimFlags.hpp"
#include "../Packets/Player/PacketPlayerAnimPlay.hpp"
#include "../Packets/Player/PacketPlayerAttack.hpp"
#include "../Packets/Player/PacketPlayerAttribute.hpp"
#include "../Packets/Player/PacketPlayerBook.hpp"
#include "../Packets/Player/PacketPlayerBounty.hpp"
#include "../Packets/Player/PacketPlayerCellChange.hpp"
#include "../Packets/Player/PacketPlayerCellState.hpp"
#include "../Packets/Player/PacketPlayerClass.hpp"
#include "../Packets/Player/PacketPlayerDeath.hpp"
#include "../Packets/Player/PacketPlayerEquipment.hpp"
#include "../Packets/Player/PacketPlayerFaction.hpp"
#include "../Packets/Player/PacketPlayerInventory.hpp"
#include "../Packets/Player/PacketPlayerJournal.hpp"
#include "../Packets/Player/PacketPlayerLevel.hpp"
#include "../Packets/Player/PacketPlayerMap.hpp"
#include "../Packets/Player/PacketPlayerPosition.hpp"
#include "../Packets/Player/PacketPlayerRegionAuthority.hpp"
#include "../Packets/Player/PacketPlayerRegionChange.hpp"
#include "../Packets/Player/PacketPlayerRest.hpp"
#include "../Packets/Player/PacketPlayerResurrect.hpp"
#include "../Packets/Player/PacketPlayerSkill.hpp"
#include "../Packets/Player/PacketPlayerSpeech.hpp"
#include "../Packets/Player/PacketPlayerSpellbook.hpp"
#include "../Packets/Player/PacketPlayerStatsDynamic.hpp"
#include "../Packets/Player/PacketPlayerTopic.hpp"

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
    AddPacket<PacketDisconnect>(&packets, peer);
    AddPacket<PacketChatMessage>(&packets, peer);
    AddPacket<PacketHandshake>(&packets, peer);
    AddPacket<PacketGUIBoxes>(&packets, peer);
    AddPacket<PacketLoaded>(&packets, peer);
    AddPacket<PacketConsole>(&packets, peer);
    AddPacket<PacketGameTime>(&packets, peer);
    AddPacket<PacketGameWeather>(&packets, peer);
    AddPacket<PacketPlayerActiveSkills>(&packets, peer);

    AddPacket<PacketPlayerBaseInfo>(&packets, peer);
    AddPacket<PacketPlayerCharGen>(&packets, peer);
    AddPacket<PacketPlayerAnimFlags>(&packets, peer);
    AddPacket<PacketPlayerAnimPlay>(&packets, peer);
    AddPacket<PacketPlayerAttack>(&packets, peer);
    AddPacket<PacketPlayerAttribute>(&packets, peer);
    AddPacket<PacketPlayerBook>(&packets, peer);
    AddPacket<PacketPlayerBounty>(&packets, peer);
    AddPacket<PacketPlayerCellChange>(&packets, peer);
    AddPacket<PacketPlayerCellState>(&packets, peer);
    AddPacket<PacketPlayerClass>(&packets, peer);
    AddPacket<PacketPlayerDeath>(&packets, peer);
    AddPacket<PacketPlayerEquipment>(&packets, peer);
    AddPacket<PacketPlayerFaction>(&packets, peer);
    AddPacket<PacketPlayerInventory>(&packets, peer);
    AddPacket<PacketPlayerJournal>(&packets, peer);
    AddPacket<PacketPlayerLevel>(&packets, peer);
    AddPacket<PacketPlayerMap>(&packets, peer);
    AddPacket<PacketPlayerPosition>(&packets, peer);
    AddPacket<PacketPlayerRegionAuthority>(&packets, peer);
    AddPacket<PacketPlayerRegionChange>(&packets, peer);
    AddPacket<PacketPlayerRest>(&packets, peer);
    AddPacket<PacketPlayerResurrect>(&packets, peer);
    AddPacket<PacketPlayerSkill>(&packets, peer);
    AddPacket<PacketPlayerSpeech>(&packets, peer);
    AddPacket<PacketPlayerSpellbook>(&packets, peer);
    AddPacket<PacketPlayerStatsDynamic>(&packets, peer);
    AddPacket<PacketPlayerTopic>(&packets, peer);
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
