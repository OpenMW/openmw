//
// Created by koncord on 12.01.16.
//

#ifndef OPENMW_PACKETDISCONNECT_HPP
#define OPENMW_PACKETDISCONNECT_HPP

#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>

namespace mwmp
{
    class PacketDisconnect : public PlayerPacket
    {
    public:
        PacketDisconnect(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
        {
            packetID = ID_USER_DISCONNECTED;
        }
    };
}


#endif //OPENMW_PACKETDISCONNECT_HPP
