#ifndef OPENMW_PACKETPLAYERREST_HPP
#define OPENMW_PACKETPLAYERREST_HPP

#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketPlayerRest : public PlayerPacket
    {
    public:
        PacketPlayerRest(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, bool send);
    };
}

#endif //OPENMW_PACKETPLAYERREST_HPP
