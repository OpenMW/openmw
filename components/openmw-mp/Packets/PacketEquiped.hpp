//
// Created by koncord on 07.01.16.
//

#ifndef OPENMW_PACKETCONTAINER_HPP
#define OPENMW_PACKETCONTAINER_HPP

#include <components/openmw-mp/Packets/BasePacket.hpp>

namespace mwmp
{
    class PacketEquiped : public BasePacket
    {
    public:
        PacketEquiped(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BasePlayer *player, bool send);
    };
}

#endif //OPENMW_PACKETCONTAINER_HPP
