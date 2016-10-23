//
// Created by koncord on 08.03.16.
//

#ifndef OPENMW_PACKETCHARGEN_HPP
#define OPENMW_PACKETCHARGEN_HPP


#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketCharGen : public PlayerPacket
    {
    public:
        PacketCharGen(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BasePlayer *player, bool send);
    };
}



#endif //OPENMW_PACKETCHARGEN_HPP
