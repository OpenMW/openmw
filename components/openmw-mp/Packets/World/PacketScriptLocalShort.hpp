#ifndef OPENMW_PACKETSCRIPTLOCALSHORT_HPP
#define OPENMW_PACKETSCRIPTLOCALSHORT_HPP

#include <components/openmw-mp/Packets/World/WorldPacket.hpp>

namespace mwmp
{
    class PacketScriptLocalShort : public WorldPacket
    {
    public:
        PacketScriptLocalShort(RakNet::RakPeerInterface *peer);

        virtual void Object(WorldObject &obj, bool send);
    };
}

#endif //OPENMW_PACKETSCRIPTLOCALSHORT_HPP
