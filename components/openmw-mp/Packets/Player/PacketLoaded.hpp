//
// Created by koncord on 17.09.16.
//

#ifndef OPENMW_PACKETLOADED_HPP
#define OPENMW_PACKETLOADED_HPP

#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketLoaded : public PlayerPacket
    {
    public:
        PacketLoaded(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
        {
            packetID = ID_LOADED;
        }
    };
}

#endif //OPENMW_PACKETLOADED_HPP
