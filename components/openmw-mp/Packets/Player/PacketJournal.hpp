#ifndef OPENMW_PACKETJOURNAL_HPP
#define OPENMW_PACKETJOURNAL_HPP

#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketJournal : public PlayerPacket
    {
    public:
        PacketJournal(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BasePlayer *player, bool send);
    };
}


#endif //OPENMW_PACKETJOURNAL_HPP
