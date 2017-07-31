//
// Created by koncord on 24.04.17.
//

#include "QueryClient.hpp"
#include <RakSleep.h>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <iostream>
#include <components/openmw-mp/Version.hpp>
#include <qdebug.h>

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
    BitStream bs;
    bs.Write((unsigned char) (ID_MASTER_QUERY));
    qDebug() << "Locking mutex in QueryClient::Query()";
    mxServers.lock();
    status = -1;
    int attempts = 3;
    do
    {
        if (Connect() == IS_NOT_CONNECTED)
        {
            qDebug() << "Unlocking mutex in QueryClient::Query()";
            mxServers.unlock();
            return query;
        }

        int code = peer->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, CHANNEL_MASTER, masterAddr, false);

        if (code == 0)
        {
            qDebug() << "Unlocking mutex in QueryClient::Query()";
            mxServers.unlock();
            return query;
        }

        pmq->SetServers(&query);
        status = GetAnswer(ID_MASTER_QUERY);
        RakSleep(100);
    }
    while(status != ID_MASTER_QUERY && attempts-- > 0);
    if(status != ID_MASTER_QUERY)
        qDebug() << "Getting query was failed";
    qDebug() << "Unlocking mutex in QueryClient::Query()";
    peer->CloseConnection(masterAddr, true);
    mxServers.unlock();
    qDebug() <<"Answer" << (status == ID_MASTER_QUERY ? "ok." : "wrong.");

    return query;
}

pair<SystemAddress, QueryData> QueryClient::Update(RakNet::SystemAddress addr)
{
    qDebug() << "Locking mutex in QueryClient::Update(RakNet::SystemAddress addr)";
    pair<SystemAddress, QueryData> server;
    BitStream bs;
    bs.Write((unsigned char) (ID_MASTER_UPDATE));
    bs.Write(addr);

    mxServers.lock();
    status = -1;
    int attempts = 3;
    pmu->SetServer(&server);
    do
    {
        if (Connect() == IS_NOT_CONNECTED)
        {
            qDebug() << IS_NOT_CONNECTED;
            qDebug() << "Unlocking mutex in QueryClient::Update(RakNet::SystemAddress addr)";
            mxServers.unlock();
            return server;
        }

        peer->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, CHANNEL_MASTER, masterAddr, false);
        status = GetAnswer(ID_MASTER_UPDATE);
        RakSleep(100);
    }
    while(status != ID_MASTER_UPDATE && attempts-- > 0);
    if(status != ID_MASTER_UPDATE)
        qDebug() << "Getting update was failed";
    peer->CloseConnection(masterAddr, true);
    qDebug() << "Unlocking mutex in QueryClient::Update(RakNet::SystemAddress addr)";
    mxServers.unlock();
    return server;
}

MASTER_PACKETS QueryClient::GetAnswer(MASTER_PACKETS waitingPacket)
{
    RakNet::Packet *packet;
    bool update = true;
    unsigned char pid = 0;
    int id = -1;
    while (update)
    {
        for (packet = peer->Receive(); packet; peer->DeallocatePacket(packet), packet = peer->Receive())
        {
            BitStream data(packet->data, packet->length, false);
            pmq->SetReadStream(&data);
            pmu->SetReadStream(&data);
            data.Read(pid);
            switch(pid)
            {
                case ID_CONNECTION_LOST:
                    qDebug() << "ID_CONNECTION_LOST";
                case ID_DISCONNECTION_NOTIFICATION:
                    qDebug() << "Disconnected";
                    update = false;
                    break;
                case ID_MASTER_QUERY:
                    qDebug() << "ID_MASTER_QUERY";
                    if (waitingPacket == ID_MASTER_QUERY)
                        pmq->Read();
                    else
                        qDebug() << "Got wrong packet";
                    update = false;
                    id = pid;
                    break;
                case ID_MASTER_UPDATE:
                    qDebug() << "ID_MASTER_UPDATE";
                    if (waitingPacket == ID_MASTER_UPDATE)
                        pmu->Read();
                    else
                        qDebug() << "Got wrong packet";
                    update = false;
                    id = pid;
                    break;
                case ID_MASTER_ANNOUNCE:
                    qDebug() << "ID_MASTER_ANNOUNCE";
                    update = false;
                    id = pid;
                    break;
                case ID_CONNECTION_REQUEST_ACCEPTED:
                    qDebug() << "ID_CONNECTION_REQUEST_ACCEPTED";
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

    ConnectionAttemptResult car = peer->Connect(masterAddr.ToString(false), masterAddr.GetPort(), TES3MP_MASTERSERVER_PASSW,
                                                strlen(TES3MP_MASTERSERVER_PASSW), 0, 0, 5, 500);

    while (true)
    {
        ConnectionState state = peer->GetConnectionState(masterAddr);
        switch (state)
        {
            case IS_CONNECTED:
                qDebug() << "Connected";
                return IS_CONNECTED;
            case IS_NOT_CONNECTED:
            case IS_DISCONNECTED:
            case IS_SILENTLY_DISCONNECTING:
            case IS_DISCONNECTING:
            {
                qDebug() << "Cannot connect to the master server. Code:"<< state;
                return IS_NOT_CONNECTED;
            }
            case IS_PENDING:
            case IS_CONNECTING:
                qDebug() << "Pending";
                break;
        }
        RakSleep(500);
    }
}

int QueryClient::Status()
{
    return status;
}
