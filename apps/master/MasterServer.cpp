//
// Created by koncord on 21.04.17.
//

#include <RakPeerInterface.h>
#include <RakSleep.h>
#include <BitStream.h>
#include <iostream>
#include "MasterServer.hpp"

#include <components/openmw-mp/Master/PacketMasterQuery.hpp>
#include <components/openmw-mp/Master/PacketMasterUpdate.hpp>
#include <components/openmw-mp/Master/PacketMasterAnnounce.hpp>

using namespace RakNet;
using namespace std;
using namespace mwmp;

MasterServer::MasterServer(unsigned short maxConnections, unsigned short port)
{
    peer = RakPeerInterface::GetInstance();
    sockdescr = SocketDescriptor(port, 0);
    peer->Startup(maxConnections, &sockdescr, 1, 1000);

    peer->SetMaximumIncomingConnections(maxConnections);
    peer->SetIncomingPassword("pass", (int) strlen("pass"));
    run = false;
}

MasterServer::~MasterServer()
{
    Stop(true);
}

using namespace chrono;

void MasterServer::Thread()
{
    unsigned char packetId = 0;

    auto startTime = chrono::steady_clock::now();

    BitStream send;
    PacketMasterQuery pmq(peer);
    pmq.SetSendStream(&send);

    PacketMasterUpdate pmu(peer);
    pmu.SetSendStream(&send);

    PacketMasterAnnounce pma(peer);
    pma.SetSendStream(&send);

    while (run)
    {
        Packet *packet = peer->Receive();

        auto now = chrono::steady_clock::now();
        if (now - startTime >= 60s)
        {
            startTime = chrono::steady_clock::now();
            for (auto it = servers.begin(); it != servers.end();)
            {

                if (it->second.lastUpdate + 60s <= now)
                    servers.erase(it++);
                else ++it;
            }
        }

        if (packet == nullptr)
            RakSleep(10);
        else
            for (; packet; peer->DeallocatePacket(packet), packet = peer->Receive())
            {
                BitStream data(packet->data, packet->length, false);
                data.Read(packetId);
                switch (packetId)
                {
                    case ID_NEW_INCOMING_CONNECTION:
                        cout << "New incoming connection: " << packet->systemAddress.ToString() << endl;
                        break;
                    case ID_DISCONNECTION_NOTIFICATION:
                        cout << "Disconnected: " << packet->systemAddress.ToString() << endl;
                        break;
                    case ID_CONNECTION_LOST:
                        cout << "Connection lost: " << packet->systemAddress.ToString() << endl;
                        break;
                    case ID_MASTER_QUERY:
                    {
                        pmq.SetServers(reinterpret_cast<map<SystemAddress, QueryData> *>(&servers));
                        pmq.Send(packet->systemAddress);

                        cout << "Sent info about all " << servers.size() << " servers to "
                             << packet->systemAddress.ToString() << endl;
                        peer->CloseConnection(packet->systemAddress, true);
                        break;
                    }
                    case ID_MASTER_UPDATE:
                    {
                        SystemAddress addr;
                        data.Read(addr); // update 1 server

                        ServerIter it = servers.find(addr);
                        if (it != servers.end())
                        {
                            pair<SystemAddress, QueryData> pairPtr(it->first, static_cast<QueryData>(it->second));
                            pmu.SetServer(&pairPtr);
                            pmu.Send(packet->systemAddress);
                            cout << "Sent info about " << addr.ToString() << " to " << packet->systemAddress.ToString()
                                 << endl;
                        }
                        peer->CloseConnection(packet->systemAddress, true);
                        break;
                    }
                    case ID_MASTER_ANNOUNCE:
                    {
                        ServerIter iter = servers.find(packet->systemAddress);

                        pma.SetReadStream(&data);
                        SServer server;
                        pma.SetServer(&server);
                        pma.Read();

                        auto keepAliveFunc = [&]() {
                            iter->second.lastUpdate = now;
                            pma.SetFunc(PacketMasterAnnounce::FUNCTION_KEEP);
                            pma.Send(packet->systemAddress);
                        };

                        if (iter != servers.end())
                        {
                            if (pma.GetFunc() == PacketMasterAnnounce::FUNCTION_DELETE)
                            {
                                servers.erase(iter);
                                cout << "Deleted";
                                pma.Send(packet->systemAddress);
                            }
                            else if (pma.GetFunc() == PacketMasterAnnounce::FUNCTION_ANNOUNCE)
                            {
                                cout << "Updated";
                                iter->second = server;
                                keepAliveFunc();
                            }
                            else
                            {
                                cout << "Keeping alive";
                                keepAliveFunc();
                            }
                        }
                        else if(pma.GetFunc() == PacketMasterAnnounce::FUNCTION_ANNOUNCE)
                        {
                            cout << "Added";
                            iter = servers.insert({packet->systemAddress, server}).first;
                            keepAliveFunc();
                        }
                        else
                        {
                            cout << "Unknown";
                            pma.SetFunc(PacketMasterAnnounce::FUNCTION_DELETE);
                            pma.Send(packet->systemAddress);
                        }
                        cout << " server " << packet->systemAddress.ToString() << endl;

                        peer->CloseConnection(packet->systemAddress, true);
                        break;
                    }
                    default:
                        cout << "Wrong packet" << endl;
                        peer->CloseConnection(packet->systemAddress, true);
                }
            }
    }
    peer->Shutdown(1000);
    RakPeerInterface::DestroyInstance(peer);
    cout << "Server thread stopped" << endl;
}

void MasterServer::Start()
{
    if (!run)
    {
        run = true;
        tMasterThread = thread(&MasterServer::Thread, this);
        cout << "Started" << endl;
    }
}

void MasterServer::Stop(bool wait)
{
    if (run)
    {
        run = false;
        if (wait && tMasterThread.joinable())
            tMasterThread.join();
    }
}

bool MasterServer::isRunning()
{
    return run;
}

void MasterServer::Wait()
{
    if (run)
    {
        if (tMasterThread.joinable())
            tMasterThread.join();
    }
}
