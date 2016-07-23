//
// Created by koncord on 23.07.16.
//

#ifndef OPENMW_PACKETGUIBOXES_HPP
#define OPENMW_PACKETGUIBOXES_HPP


#include <components/openmw-mp/Packets/BasePacket.hpp>

namespace mwmp
{
    class PacketGUIBoxes : public BasePacket
    {
    public:
        PacketGUIBoxes(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BasePlayer *player, bool send);
    };
}

#endif //OPENMW_PACKETGUIBOXES_HPP
