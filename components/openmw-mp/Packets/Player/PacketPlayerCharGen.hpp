//
// Created by koncord on 08.03.16.
//

#ifndef OPENMW_PACKETPLAYERCHARGEN_HPP
#define OPENMW_PACKETPLAYERCHARGEN_HPP


#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketPlayerCharGen : public PlayerPacket
    {
    public:
        PacketPlayerCharGen(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BasePlayer *player, bool send);
    };
}



#endif //OPENMW_PACKETPLAYERCHARGEN_HPP
