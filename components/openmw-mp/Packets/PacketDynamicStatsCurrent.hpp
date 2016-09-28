//
// Created by David Cernat on 28.09.16.
//

#ifndef OPENMW_PACKAGEDYNAMICSTATSCURRENT_HPP
#define OPENMW_PACKAGEDYNAMICSTATSCURRENT_HPP


#include <components/openmw-mp/Packets/BasePacket.hpp>

namespace mwmp
{
    class PacketDynamicStatsCurrent : public BasePacket
    {
    public:
        PacketDynamicStatsCurrent(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BasePlayer *player, bool send);
    };
}

#endif //OPENMW_PACKAGEDYNAMICSTATSCURRENT_HPP
