//
// Created by koncord on 22.10.16.
//

#ifndef OPENMW_PACKETINVENTORY_HPP
#define OPENMW_PACKETINVENTORY_HPP

#include <components/openmw-mp/Packets/PlayerPacket.hpp>

namespace mwmp
{
    class PacketInventory : public PlayerPacket
    {
    public:
        PacketInventory(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BasePlayer *player, bool send);
    };
}


#endif //OPENMW_PACKETINVENTORY_HPP
