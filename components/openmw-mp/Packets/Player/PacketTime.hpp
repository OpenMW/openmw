//
// Created by koncord on 30.08.16.
//

#ifndef OPENMW_TIMEPACKET_HPP
#define OPENMW_TIMEPACKET_HPP


#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketTime : public PlayerPacket
    {
    public:
        PacketTime(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BasePlayer *player, bool send);
    };
}


#endif //OPENMW_TIMEPACKET_HPP
