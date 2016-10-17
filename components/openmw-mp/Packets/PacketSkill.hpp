//
// Created by koncord on 17.03.16.
//

#ifndef OPENMW_PACKETSKILL_HPP
#define OPENMW_PACKETSKILL_HPP


#include <components/openmw-mp/Packets/PlayerPacket.hpp>

namespace mwmp
{
    class PacketSkill : public PlayerPacket
    {
    public:
        const static int SkillCount = 27;
        const static int AttributeCount = 8;
        PacketSkill(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BasePlayer *player, bool send);
    };
}



#endif //OPENMW_PACKETSKILL_HPP
