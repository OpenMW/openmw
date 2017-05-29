#ifndef OPENMW_PACKETPLAYERFACTION_HPP
#define OPENMW_PACKETPLAYERFACTION_HPP

#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketPlayerFaction : public PlayerPacket
    {
    public:
        PacketPlayerFaction(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, bool send);
    };
}

#endif //OPENMW_PACKETPLAYERFACTION_HPP
