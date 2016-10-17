//
// Created by koncord on 13.01.16.
//

#ifndef OPENMW_PACKETATTACK_HPP
#define OPENMW_PACKETATTACK_HPP


#include <components/openmw-mp/Packets/PlayerPacket.hpp>

namespace mwmp
{
    class PacketAttack : public PlayerPacket
    {
    public:
        PacketAttack(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BasePlayer *player, bool send);
    };
}

#endif //OPENMW_PACKETATTACK_HPP
