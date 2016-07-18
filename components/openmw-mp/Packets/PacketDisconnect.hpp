//
// Created by koncord on 12.01.16.
//

#ifndef OPENMW_PACKETDISCONNECT_HPP
#define OPENMW_PACKETDISCONNECT_HPP

#include <components/openmw-mp/Packets/BasePacket.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>

namespace mwmp
{
    class PacketDisconnect : public BasePacket
    {
    public:
        PacketDisconnect(RakNet::RakPeerInterface *peer) : BasePacket(peer)
        {
            packetID = ID_USER_DISCONNECTED;
        }
    };
}


#endif //OPENMW_PACKETDISCONNECT_HPP
