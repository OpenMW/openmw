//
// Created by koncord on 22.04.17.
//

#ifndef OPENMW_PACKETMASTERQUERY_HPP
#define OPENMW_PACKETMASTERQUERY_HPP

#include "../Packets/BasePacket.hpp"
#include "MasterData.hpp"

namespace mwmp
{
    class ProxyMasterPacket;
    class PacketMasterQuery : public BasePacket
    {
        friend class ProxyMasterPacket;
    public:
        PacketMasterQuery(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, bool send);

        void SetServers(std::map<RakNet::SystemAddress, QueryData> *serverMap);
    private:
        std::map<RakNet::SystemAddress, QueryData> *servers;
    };
}

#endif //OPENMW_PACKETMASTERQUERY_HPP
