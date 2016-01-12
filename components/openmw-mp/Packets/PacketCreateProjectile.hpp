//
// Created by koncord on 13.01.16.
//

#ifndef OPENMW_PACKETCREATEPROJECTILE_HPP
#define OPENMW_PACKETCREATEPROJECTILE_HPP


#include <components/openmw-mp/Packets/BasePacket.hpp>

namespace mwmp
{
    class PacketCreateProjectile : public BasePacket
    {
    public:
        PacketCreateProjectile(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BasePlayer *player, bool send);
    };
}


#endif //OPENMW_PACKETCREATEPROJECTILE_HPP
