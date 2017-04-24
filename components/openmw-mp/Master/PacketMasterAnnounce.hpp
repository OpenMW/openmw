//
// Created by koncord on 22.04.17.
//

#ifndef OPENMW_PACKETMASTERANNOUNCE_HPP
#define OPENMW_PACKETMASTERANNOUNCE_HPP

#include "../Packets/BasePacket.hpp"
#include "MasterData.hpp"

namespace mwmp
{
    class ProxyMasterPacket;
    class PacketMasterAnnounce : public BasePacket
    {
        friend class ProxyMasterPacket;
    public:
        PacketMasterAnnounce(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, bool send);

        void SetServer(QueryData *server);
        void SetFunc(int keep);
        int GetFunc();

        enum Func
        {
            FUNCTION_DELETE = 0,
            FUNCTION_ANNOUNCE,
            FUNCTION_KEEP
        };
    private:
        QueryData *server;
        int func;
    };
}

#endif //OPENMW_PACKETMASTERANNOUNCE_HPP
