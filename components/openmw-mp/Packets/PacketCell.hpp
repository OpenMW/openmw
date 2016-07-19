//
// Created by koncord on 15.01.16.
//

#ifndef OPENMW_PACKETCELL_HPP
#define OPENMW_PACKETCELL_HPP


#include <components/openmw-mp/Packets/BasePacket.hpp>

namespace mwmp
{
    class PacketCell : public BasePacket
    {
    public:
        PacketCell(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BasePlayer *player, bool send);
    };
}



#endif //OPENMW_PACKETCELL_HPP
