//
// Created by koncord on 21.04.17.
//

#include <RakPeerInterface.h>
#include <RakSleep.h>
#include <BitStream.h>
#include <iostream>
#include <Kbhit.h>
#include <Gets.h>
#include <components/openmw-mp/Master/MasterData.hpp>
#include <components/openmw-mp/Master/PacketMasterAnnounce.hpp>
#include <components/openmw-mp/Master/PacketMasterUpdate.hpp>
#include <components/openmw-mp/Master/PacketMasterQuery.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>

using namespace std;
using namespace RakNet;
using namespace mwmp;

int main()
{
    cout << "Server test" << endl;

    SystemAddress masterAddr("127.0.0.1", 25560);

    RakPeerInterface *peer = RakNet::RakPeerInterface::GetInstance();

    RakNet::SocketDescriptor sd(25565, 0);
    peer->Startup(8, &sd, 1);

    ConnectionAttemptResult result = peer->Connect(masterAddr.ToString(false), masterAddr.GetPort(), "pass",
                                                   (int)(strlen("pass")), 0, 0, 5, 500);

    assert(result == RakNet::CONNECTION_ATTEMPT_STARTED);

    char message[2048];
    BitStream send;

    PacketMasterQuery pmq(peer);
    pmq.SetSendStream(&send);

    PacketMasterAnnounce pma(peer);
    pma.SetSendStream(&send);

    while (true)
    {
        RakSleep(30);

        if (kbhit())
        {
            Gets(message, sizeof(message));

            if (strcmp(message, "quit") == 0)
            {
                puts("Quitting.");
                break;
            }
            else if (strcmp(message, "send") == 0)
            {
                puts("Sending data about server");
                QueryData server;
                server.SetName("Super Server");
                server.SetPlayers(0);
                server.SetMaxPlayers(0);

                pma.SetServer(&server);
                pma.SetFunc(PacketMasterAnnounce::FUNCTION_ANNOUNCE);
                pma.Send(masterAddr);
            }
            else if (strcmp(message, "get") == 0)
            {
                puts("Request query info");
                send.Reset();
                send.Write((unsigned char) (ID_MASTER_QUERY));
                peer->Send(&send, HIGH_PRIORITY, RELIABLE_ORDERED, CHANNEL_MASTER, masterAddr, false);
            }
            else if (strcmp(message, "getme") == 0)
            {
                send.Reset();
                send.Write((unsigned char) (ID_MASTER_UPDATE));
                send.Write(SystemAddress("127.0.0.1", 25565));
                peer->Send(&send, HIGH_PRIORITY, RELIABLE_ORDERED, CHANNEL_MASTER, masterAddr, false);
            }
            else if (strcmp(message, "status") == 0)
            {
                cout << (peer->GetConnectionState(masterAddr) == IS_CONNECTED ? "Connected" : "Not connected") << endl;
            }
            else if (strcmp(message, "keep") == 0)
            {
                cout << "Sending keep alive" << endl;
                pma.SetFunc(PacketMasterAnnounce::FUNCTION_KEEP);
                pma.Send(masterAddr);
            }
        }

        for (RakNet::Packet *packet = peer->Receive(); packet; peer->DeallocatePacket(packet), packet = peer->Receive())
        {
            BitStream data(packet->data, packet->length, false);
            unsigned char packetID;
            data.Read(packetID);
            switch (packetID)
            {
                case ID_DISCONNECTION_NOTIFICATION:
                    // Connection lost normally
                    printf("ID_DISCONNECTION_NOTIFICATION\n");
                    break;
                case ID_ALREADY_CONNECTED:
                    // Connection lost normally
                    printf("ID_ALREADY_CONNECTED with guid %lu\n", packet->guid.g);
                    break;
                case ID_INCOMPATIBLE_PROTOCOL_VERSION:
                    printf("ID_INCOMPATIBLE_PROTOCOL_VERSION\n");
                    break;
                case ID_REMOTE_DISCONNECTION_NOTIFICATION: // Server telling the clients of another client disconnecting gracefully.  You can manually broadcast this in a peer to peer enviroment if you want.
                    printf("ID_REMOTE_DISCONNECTION_NOTIFICATION\n");
                    break;
                case ID_REMOTE_CONNECTION_LOST: // Server telling the clients of another client disconnecting forcefully.  You can manually broadcast this in a peer to peer enviroment if you want.
                    printf("ID_REMOTE_CONNECTION_LOST\n");
                    break;
                case ID_REMOTE_NEW_INCOMING_CONNECTION: // Server telling the clients of another client connecting.  You can manually broadcast this in a peer to peer enviroment if you want.
                    printf("ID_REMOTE_NEW_INCOMING_CONNECTION\n");
                    break;
                case ID_CONNECTION_BANNED: // Banned from this server
                    printf("We are banned from this server.\n");
                    break;
                case ID_CONNECTION_ATTEMPT_FAILED:
                    printf("Connection attempt failed\n");
                    break;
                case ID_NO_FREE_INCOMING_CONNECTIONS:
                    // Sorry, the server is full.  I don't do anything here but
                    // A real app should tell the user
                    printf("ID_NO_FREE_INCOMING_CONNECTIONS\n");
                    break;

                case ID_INVALID_PASSWORD:
                    printf("ID_INVALID_PASSWORD\n");
                    break;

                case ID_CONNECTION_LOST:
                    // Couldn't deliver a reliable packet - i.e. the other system was abnormally
                    // terminated
                    printf("ID_CONNECTION_LOST\n");
                    return 0;
                    break;

                case ID_CONNECTION_REQUEST_ACCEPTED:
                    // This tells the client they have connected
                    printf("ID_CONNECTION_REQUEST_ACCEPTED to %s with GUID %s\n", packet->systemAddress.ToString(true),
                           packet->guid.ToString());
                    printf("My external address is %s\n", peer->GetExternalID(packet->systemAddress).ToString(true));
                    break;
                case ID_MASTER_QUERY:
                {
                    map<SystemAddress, QueryData> servers;

                    pmq.SetReadStream(&data);
                    pmq.SetServers(&servers);
                    pmq.Read();

                    cout << "Received query data about " << servers.size() << " servers" << endl;

                    for (auto serv : servers)
                        cout << serv.second.GetName() << endl;

                    break;
                }
                case ID_MASTER_UPDATE:
                {
                    pair<SystemAddress, QueryData> serverPair;
                    PacketMasterUpdate pmu(peer);
                    pmu.SetReadStream(&data);
                    pmu.SetServer(&serverPair);
                    pmu.Read();
                    cout << "Received info about " << serverPair.first.ToString() << endl;
                    cout << serverPair.second.GetName() << endl;
                    break;
                }
                default:
                    cout << "Wrong packet" << endl;
            }
        }
    }
    peer->Shutdown(1000);
    RakPeerInterface::DestroyInstance(peer);
}