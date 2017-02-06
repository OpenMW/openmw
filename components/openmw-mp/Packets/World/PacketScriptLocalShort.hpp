#ifndef OPENMW_PACKETSCRIPTLOCALSHORT_HPP
#define OPENMW_PACKETSCRIPTLOCALSHORT_HPP

#include <components/openmw-mp/Packets/World/WorldPacket.hpp>

namespace mwmp
{
    class PacketScriptLocalShort : public WorldPacket
    {
    public:
        PacketScriptLocalShort(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BaseEvent *event, bool send);
    };
}

#endif //OPENMW_PACKETSCRIPTLOCALSHORT_HPP
