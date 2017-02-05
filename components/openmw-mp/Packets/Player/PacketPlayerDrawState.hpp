//
// Created by koncord on 15.01.16.
//

#ifndef OPENMW_PACKETPLAYERDRAWSTATE_HPP
#define OPENMW_PACKETPLAYERDRAWSTATE_HPP


#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketPlayerDrawState : public PlayerPacket
    {
    public:
        PacketPlayerDrawState(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BasePlayer *player, bool send);
    };
}


#endif //OPENMW_PACKETPLAYERDRAWSTATE_HPP
