//
// Created by koncord on 17.03.16.
//

#ifndef OPENMW_PACKETSKILL_HPP
#define OPENMW_PACKETSKILL_HPP


#include <components/openmw-mp/Packets/BasePacket.hpp>

namespace mwmp
{
    class PacketSkill : public BasePacket
    {
    public:
        const static int StatsCount = 27;
        PacketSkill(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BasePlayer *player, bool send);
    };
}



#endif //OPENMW_PACKETSKILL_HPP
