#ifndef OPENMW_PACKETSPELLBOOK_HPP
#define OPENMW_PACKETSPELLBOOK_HPP

#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketSpellbook : public PlayerPacket
    {
    public:
        PacketSpellbook(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BasePlayer *player, bool send);
    };
}


#endif //OPENMW_PACKETSPELLBOOK_HPP
