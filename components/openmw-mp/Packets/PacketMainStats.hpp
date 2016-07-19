//
// Created by koncord on 13.01.16.
//

#ifndef OPENMW_PACKAGEMAINSTATS_HPP
#define OPENMW_PACKAGEMAINSTATS_HPP


#include <components/openmw-mp/Packets/BasePacket.hpp>

namespace mwmp
{
    class PacketMainStats : public BasePacket
    {
    public:
        PacketMainStats(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BasePlayer *player, bool send);
    };
}

#endif //OPENMW_PACKAGEMAINSTATS_HPP
