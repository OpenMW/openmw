#ifndef OPENMW_PACKETGAMESETTINGS_HPP
#define OPENMW_PACKETGAMESETTINGS_HPP

#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>

namespace mwmp
{
    class PacketGameSettings: public PlayerPacket
    {
    public:
        PacketGameSettings(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, bool send);
    };
}

#endif //OPENMW_PACKETGAMESETTINGS_HPP
