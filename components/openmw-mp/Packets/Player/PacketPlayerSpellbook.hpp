#ifndef OPENMW_PACKETPLAYERSPELLBOOK_HPP
#define OPENMW_PACKETPLAYERSPELLBOOK_HPP

#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketPlayerSpellbook : public PlayerPacket
    {
    public:
        PacketPlayerSpellbook(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BasePlayer *player, bool send);
    };
}


#endif //OPENMW_PACKETPLAYERSPELLBOOK_HPP
