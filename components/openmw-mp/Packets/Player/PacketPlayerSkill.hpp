//
// Created by koncord on 17.03.16.
//

#ifndef OPENMW_PACKETPLAYERSKILL_HPP
#define OPENMW_PACKETPLAYERSKILL_HPP


#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketPlayerSkill : public PlayerPacket
    {
    public:
        const static int SkillCount = 27;
        const static int AttributeCount = 8;
        PacketPlayerSkill(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BasePlayer *player, bool send);
    };
}



#endif //OPENMW_PACKETPLAYERSKILL_HPP
