//
// Created by koncord on 24.04.17.
//

#include "QueryClient.hpp"
#include <RakSleep.h>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <iostream>

using namespace RakNet;
using namespace std;
using namespace mwmp;

QueryClient::QueryClient()
{
    peer = RakPeerInterface::GetInstance();
    pmq = new PacketMasterQuery(peer);
    pmu = new PacketMasterUpdate(peer);
    RakNet::SocketDescriptor sd;
    peer->Startup(8, &sd, 1);
    status = -1;
}

QueryClient::~QueryClient()
{
    delete pmq;
    delete pmu;
    RakPeerInterface::DestroyInstance(peer);
}

void QueryClient::SetServer(std::string addr, unsigned short port)
{
    masterAddr = SystemAddress(addr.c_str(), port);
}

QueryClient &QueryClient::Get()
{
    static QueryClient myInstance;
    return myInstance;
}

map<SystemAddress, QueryData> QueryClient::Query()
{
    map<SystemAddress, QueryData> query;
    if (Connect() == IS_NOT_CONNECTED)
    {
        status = -1;
        return query;
    }

    BitStream bs;
    bs.Write((unsigned char) (ID_MASTER_QUERY));
    peer->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, CHANNEL_MASTER, masterAddr, false);

    pmq->SetServers(&query);
    status = GetAnswer();
    return query;
}

pair<SystemAddress, QueryData> QueryClient::Update(RakNet::SystemAddress addr)
{
    pair<SystemAddress, QueryData> server;
    if (Connect() == IS_NOT_CONNECTED)
    {
        status = -1;
        return server;
    }

    BitStream bs;
    bs.Write((unsigned char) (ID_MASTER_UPDATE));
    bs.Write(addr);
    peer->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, CHANNEL_MASTER, masterAddr, false);

    pmu->SetServer(&server);
    status = GetAnswer();
    return server;
}

MASTER_PACKETS QueryClient::GetAnswer()
{
    RakNet::Packet *packet;
    bool update = true;
    unsigned char pid = 0;
    int id;
    while(update)
    {
        for (packet = peer->Receive(); packet; peer->DeallocatePacket(packet), packet = peer->Receive())
        {
            BitStream data(packet->data, packet->length, false);
            pmq->SetReadStream(&data);
            pmu->SetReadStream(&data);
            data.Read(pid);
            switch(pid)
            {
                case ID_MASTER_QUERY:
                    pmq->Read();
                    update = false;
                    id = pid;
                    break;
                case ID_MASTER_UPDATE:
                    pmu->Read();
                    update = false;
                    id = pid;
                    break;
                case ID_MASTER_ANNOUNCE:
                    update = false;
                    id = pid;
                    break;
                default:
                    break;
            }
        }
        RakSleep(500);
    }
    return (MASTER_PACKETS)(id);
}

ConnectionState QueryClient::Connect()
{
    ConnectionAttemptResult car = peer->Connect(masterAddr.ToString(false), masterAddr.GetPort(), "pass", strlen("pass"), 0, 0, 5, 500);

    while (true)
    {
        ConnectionState state = peer->GetConnectionState(masterAddr);
        switch (state)
        {
            case IS_CONNECTED:
                return IS_CONNECTED;
            case IS_NOT_CONNECTED:
            case IS_DISCONNECTED:
            case IS_SILENTLY_DISCONNECTING:
            case IS_DISCONNECTING:
            {
                //LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Cannot connect to master server: %d", masterAddr.ToString());
                return IS_NOT_CONNECTED;
            }
            case IS_PENDING:
            case IS_CONNECTING:
                break;
        }
        RakSleep(500);
    }
}

int QueryClient::Status()
{
    return status;
}
