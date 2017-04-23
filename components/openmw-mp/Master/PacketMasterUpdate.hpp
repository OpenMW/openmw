//
// Created by koncord on 22.04.17.
//

#ifndef OPENMW_PACKETMASTERUPDATE_HPP
#define OPENMW_PACKETMASTERUPDATE_HPP

#include "../Packets/BasePacket.hpp"
#include "MasterData.hpp"

namespace mwmp
{
    class ProxyMasterPacket;
    class PacketMasterUpdate : public BasePacket
    {
        friend class ProxyMasterPacket;
    public:
        PacketMasterUpdate(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, bool send);

        void SetServer(std::pair<RakNet::SystemAddress, Server> *serverPair);
    private:
        std::pair<RakNet::SystemAddress, Server> *server;
    };
}

#endif //OPENMW_PACKETMASTERUPDATE_HPP
