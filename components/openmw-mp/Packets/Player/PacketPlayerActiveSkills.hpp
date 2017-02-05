//
// Created by koncord on 03.12.16.
//

#ifndef OPENMW_PACKETPLAYERACTIVESKILLS_HPP
#define OPENMW_PACKETPLAYERACTIVESKILLS_HPP

#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketPlayerActiveSkills : public PlayerPacket
    {
    public:
        PacketPlayerActiveSkills(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BasePlayer *player, bool send);
    };
}

#endif //OPENMW_PACKETPLAYERACTIVESKILLS_HPP
