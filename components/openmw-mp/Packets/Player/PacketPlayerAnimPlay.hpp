#ifndef OPENMW_PACKETPLAYERANIMPLAY_HPP
#define OPENMW_PACKETPLAYERANIMPLAY_HPP

#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketPlayerAnimPlay : public PlayerPacket
    {
    public:
        PacketPlayerAnimPlay(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, bool send);
    };
}

#endif //OPENMW_PACKETPLAYERANIMPLAY_HPP
