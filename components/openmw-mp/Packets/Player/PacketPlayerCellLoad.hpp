#ifndef OPENMW_PACKETPLAYERCELLLOAD_HPP
#define OPENMW_PACKETPLAYERCELLLOAD_HPP


#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketPlayerCellLoad : public PlayerPacket
    {
    public:
        PacketPlayerCellLoad(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BasePlayer *player, bool send);
    };
}



#endif //OPENMW_PACKETPLAYERCELLLOAD_HPP
