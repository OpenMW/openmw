#ifndef OPENMW_PACKETPLAYERSPEECH_HPP
#define OPENMW_PACKETPLAYERSPEECH_HPP

#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketPlayerSpeech : public PlayerPacket
    {
    public:
        PacketPlayerSpeech(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, bool send);
    };
}

#endif //OPENMW_PACKETPLAYERSPEECH_HPP
