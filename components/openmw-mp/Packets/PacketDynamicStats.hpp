//
// Created by koncord on 13.01.16.
//

#ifndef OPENMW_PACKAGEDYNAMICSTATS_HPP
#define OPENMW_PACKAGEDYNAMICSTATS_HPP


#include <components/openmw-mp/Packets/BasePacket.hpp>

namespace mwmp
{
    class PacketDynamicStats : public BasePacket
    {
    public:
        PacketDynamicStats(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BasePlayer *player, bool send);
    };
}

#endif //OPENMW_PACKAGEDYNAMICSTATS_HPP
