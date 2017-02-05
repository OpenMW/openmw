//
// Created by koncord on 13.01.16.
//

#ifndef OPENMW_PACKETDYNAMICSTATS_HPP
#define OPENMW_PACKETDYNAMICSTATS_HPP


#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketPlayerDynamicStats : public PlayerPacket
    {
    public:
        PacketPlayerDynamicStats(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BasePlayer *player, bool send);
    };
}

#endif //OPENMW_PACKETDYNAMICSTATS_HPP
