#ifndef OPENMW_PACKETPLAYERTOPIC_HPP
#define OPENMW_PACKETPLAYERTOPIC_HPP

#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketPlayerTopic : public PlayerPacket
    {
    public:
        PacketPlayerTopic(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, bool send);
    };
}


#endif //OPENMW_PACKETPLAYERTOPIC_HPP
