//
// Created by koncord on 22.04.17.
//

#include <components/openmw-mp/NetworkMessages.hpp>
#include <iostream>
#include "MasterData.hpp"
#include "PacketMasterQuery.hpp"
#include "ProxyMasterPacket.hpp"


using namespace mwmp;
using namespace std;
using namespace RakNet;

PacketMasterQuery::PacketMasterQuery(RakNet::RakPeerInterface *peer) : BasePacket(peer)
{
    packetID = ID_MASTER_QUERY;
    orderChannel = CHANNEL_MASTER;
}

void PacketMasterQuery::Packet(RakNet::BitStream *bs, bool send)
{
    this->bs = bs;
    if (send)
        bs->Write(packetID);

    int serversCount = servers->size();

    RW(serversCount, send);

    map<SystemAddress, Server>::iterator serverIt;
    if(send)
        serverIt = servers->begin();

    Server server;
    SystemAddress sa;
    while(serversCount--)
    {
        if(send)
        {
            sa = serverIt->first;
            server = serverIt->second;
        }

        RW(sa, send);
        ProxyMasterPacket::addServer(this, server, send);

        if(send)
            serverIt++;
        else
            servers->insert(pair<SystemAddress, Server>(sa, server));
    }

}

void PacketMasterQuery::SetServers(map<SystemAddress, Server> *serverMap)
{
    servers = serverMap;
}
