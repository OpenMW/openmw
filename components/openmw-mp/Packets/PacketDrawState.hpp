//
// Created by koncord on 15.01.16.
//

#ifndef OPENMW_PACKETDRAWSTATE_HPP
#define OPENMW_PACKETDRAWSTATE_HPP


#include <components/openmw-mp/Packets/PlayerPacket.hpp>

namespace mwmp
{
    class PacketDrawState : public PlayerPacket
    {
    public:
        PacketDrawState(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BasePlayer *player, bool send);
    };
}


#endif //OPENMW_PACKETDRAWSTATE_HPP
