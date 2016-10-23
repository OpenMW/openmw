//
// Created by koncord on 23.07.16.
//

#ifndef OPENMW_PACKETGUIBOXES_HPP
#define OPENMW_PACKETGUIBOXES_HPP


#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketGUIBoxes : public PlayerPacket
    {
    public:
        PacketGUIBoxes(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BasePlayer *player, bool send);
    };
}

#endif //OPENMW_PACKETGUIBOXES_HPP
