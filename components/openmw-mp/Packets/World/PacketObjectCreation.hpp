#ifndef OPENMW_PACKETOBJECTCREATION_HPP
#define OPENMW_PACKETOBJECTCREATION_HPP

#include <components/openmw-mp/Packets/World/WorldPacket.hpp>

namespace mwmp
{
    class PacketObjectCreation : public WorldPacket
    {
    public:
        PacketObjectCreation(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, WorldEvent *event, bool send);
    };
}

#endif //OPENMW_PACKETOBJECTCREATION_HPP
