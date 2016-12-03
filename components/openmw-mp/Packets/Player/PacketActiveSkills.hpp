//
// Created by koncord on 03.12.16.
//

#ifndef OPENMW_PACKETACTIVESKILLS_HPP
#define OPENMW_PACKETACTIVESKILLS_HPP

#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketActiveSkills : public PlayerPacket
    {
    public:
        PacketActiveSkills(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BasePlayer *player, bool send);
    };
}

#endif //OPENMW_PACKETACTIVESKILLS_HPP
