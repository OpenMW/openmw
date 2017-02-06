#ifndef OPENMW_PACKETMUSICPLAY_HPP
#define OPENMW_PACKETMUSICPLAY_HPP

#include <components/openmw-mp/Packets/World/WorldPacket.hpp>

namespace mwmp
{
    class PacketMusicPlay : public WorldPacket
    {
    public:
        PacketMusicPlay(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BaseEvent *event, bool send);
    };
}

#endif //OPENMW_PACKETMUSICPLAY_HPP
