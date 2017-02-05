#ifndef OPENMW_PACKETPLAYERJOURNAL_HPP
#define OPENMW_PACKETPLAYERJOURNAL_HPP

#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketPlayerJournal : public PlayerPacket
    {
    public:
        PacketPlayerJournal(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BasePlayer *player, bool send);
    };
}


#endif //OPENMW_PACKETPLAYERJOURNAL_HPP
