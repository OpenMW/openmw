//
// Created by koncord on 30.08.16.
//

#ifndef OPENMW_TIMEPACKET_HPP
#define OPENMW_TIMEPACKET_HPP


#include <components/openmw-mp/Packets/BasePacket.hpp>

namespace mwmp
{
    class PacketTime : public BasePacket
    {
    public:
        const static int StatsCount = 27;
        PacketTime(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BasePlayer *player, bool send);
    };
}


#endif //OPENMW_TIMEPACKET_HPP
