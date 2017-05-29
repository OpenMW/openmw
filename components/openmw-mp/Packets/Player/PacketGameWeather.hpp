#ifndef OPENMW_PACKETGAMEWEATHER_HPP
#define OPENMW_PACKETGAMEWEATHER_HPP

#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketGameWeather : public PlayerPacket
    {
    public:
        PacketGameWeather(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, bool send);
    };
}

#endif //OPENMW_PACKETGAMEWEATHER_HPP
