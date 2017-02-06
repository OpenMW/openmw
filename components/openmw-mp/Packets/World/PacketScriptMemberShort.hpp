#ifndef OPENMW_PACKETSCRIPTMEMBERSHORT_HPP
#define OPENMW_PACKETSCRIPTMEMBERSHORT_HPP

#include <components/openmw-mp/Packets/World/WorldPacket.hpp>

namespace mwmp
{
    class PacketScriptMemberShort : public WorldPacket
    {
    public:
        PacketScriptMemberShort(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BaseEvent *event, bool send);
    };
}

#endif //OPENMW_PACKETSCRIPTMEMBERSHORT_HPP
