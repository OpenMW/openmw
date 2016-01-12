//
// Created by koncord on 08.03.16.
//

#ifndef OPENMW_PACKETCHARGEN_HPP
#define OPENMW_PACKETCHARGEN_HPP


#include <components/openmw-mp/Packets/BasePacket.hpp>

namespace mwmp
{
    class PacketCharGen : public BasePacket
    {
    public:
        PacketCharGen(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BasePlayer *player, bool send);
    };
}



#endif //OPENMW_PACKETCHARGEN_HPP
