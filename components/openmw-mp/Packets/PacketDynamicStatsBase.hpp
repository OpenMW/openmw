//
// Created by David Cernat on 28.09.16.
//

#ifndef OPENMW_PACKAGEDYNAMICSTATSBASE_HPP
#define OPENMW_PACKAGEDYNAMICSTATSBASE_HPP


#include <components/openmw-mp/Packets/BasePacket.hpp>

namespace mwmp
{
    class PacketDynamicStatsBase : public BasePacket
    {
    public:
        PacketDynamicStatsBase(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BasePlayer *player, bool send);
    };
}

#endif //OPENMW_PACKAGEDYNAMICSTATSBASE_HPP
