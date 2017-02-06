#ifndef OPENMW_PACKETVIDEOPLAY_HPP
#define OPENMW_PACKETVIDEOPLAY_HPP

#include <components/openmw-mp/Packets/World/WorldPacket.hpp>

namespace mwmp
{
    class PacketVideoPlay : public WorldPacket
    {
    public:
        PacketVideoPlay(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BaseEvent *event, bool send);
    };
}

#endif //OPENMW_PACKETVIDEOPLAY_HPP
