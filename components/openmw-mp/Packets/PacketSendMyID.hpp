//
// Created by koncord on 12.01.16.
//

#ifndef OPENMW_PACKETSENDMYID_HPP
#define OPENMW_PACKETSENDMYID_HPP

#include <components/openmw-mp/Packets/BasePacket.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>

namespace mwmp
{
    class PacketSendMyID : public BasePacket
    {
    public:
        PacketSendMyID(RakNet::RakPeerInterface *peer) : BasePacket(peer)
        {
            packetID = ID_USER_MYID;
        }
    };
}


#endif //OPENMW_PACKETSENDMYID_HPP
