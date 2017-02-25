//
// Created by koncord on 07.01.17.
//

#include <RakPeer.h>
#include <MessageIdentifiers.h>
#include <RakSleep.h>
#include <GetTime.h>

#include <iostream>
#include <sstream>
#include <components/openmw-mp/Version.hpp>

#include "Utils.hpp"

using namespace std;

unsigned int PingRakNetServer(const char *addr, unsigned short port)
{
    RakNet::Packet *packet;
    bool done = false;
    int attempts = 0;
    RakNet::TimeMS time = PING_UNREACHABLE;

    RakNet::SocketDescriptor socketDescriptor {0, ""};
    RakNet::RakPeerInterface *peer =  RakNet::RakPeerInterface::GetInstance();
    peer->Startup(1,&socketDescriptor, 1);

    peer->Ping(addr, port, false);
    while (!done)
    {
        for (packet=peer->Receive(); packet; peer->DeallocatePacket(packet), packet=peer->Receive())
        {
            if(packet->data[0] == ID_UNCONNECTED_PONG)
            {
                    RakNet::BitStream bsIn(&packet->data[1], packet->length, false);
                    bsIn.Read(time);
                    time = RakNet::GetTimeMS() - time - 5;
                    done = true;
                    break;
            }
        }

        if (attempts >= 60) // wait 300 msec
            done = true;
        attempts++;
        RakSleep(5);
    }

    peer->Shutdown(0);
    RakNet::RakPeerInterface::DestroyInstance(peer);
    return time;
}

ServerExtendedData getExtendedData(const char *addr, unsigned short port)
{
    ServerExtendedData data;
    RakNet::SocketDescriptor socketDescriptor = {0, ""};
    RakNet::RakPeerInterface *peer = RakNet::RakPeerInterface::GetInstance();
    peer->Startup(1, &socketDescriptor, 1);

    stringstream sstr(TES3MP_VERSION);
    sstr << TES3MP_PROTO_VERSION;

    std::string msg = "";

    if (peer->Connect(addr, port, sstr.str().c_str(), (int)(sstr.str().size()), 0, 0, 3, 500, 0) != RakNet::CONNECTION_ATTEMPT_STARTED)
        msg = "Connection attempt failed.\n";


    int queue = 0;
    while (queue == 0)
    {
        for (RakNet::Packet *packet = peer->Receive(); packet; peer->DeallocatePacket(
                packet), packet = peer->Receive())
        {
            switch (packet->data[0])
            {
                case ID_CONNECTION_ATTEMPT_FAILED:
                {
                    msg = "Connection failed.\n"
                            "Either the IP address is wrong or a firewall on either system is blocking\n"
                            "UDP packets on the port you have chosen.";
                    queue = -1;
                    break;
                }
                case ID_INVALID_PASSWORD:
                {
                    msg = "Connection failed.\n"
                            "The client or server is outdated.\n";
                    queue = -1;
                    break;
                }
                case ID_CONNECTION_REQUEST_ACCEPTED:
                {
                    msg = "Connection accepted.\n";
                    queue = 1;
                    break;
                }
                case ID_DISCONNECTION_NOTIFICATION:
                    throw runtime_error("ID_DISCONNECTION_NOTIFICATION.\n");
                case ID_CONNECTION_BANNED:
                    throw runtime_error("ID_CONNECTION_BANNED.\n");
                case ID_CONNECTION_LOST:
                    throw runtime_error("ID_CONNECTION_LOST.\n");
                default:
                    printf("Connection message with identifier %i has arrived in initialization.\n", packet->data[0]);
            }
        }
    }
    puts(msg.c_str());

    if(queue == -1) // connection is failed
        return data;

    {
        RakNet::BitStream bs;
        bs.Write((unsigned char) (ID_USER_PACKET_ENUM + 1));
        peer->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
    }

    RakNet::Packet *packet;
    bool done = false;
    while (!done)
    {
        for (packet = peer->Receive(); packet; peer->DeallocatePacket(packet), packet = peer->Receive())
        {
            if(packet->data[0] == (ID_USER_PACKET_ENUM+1))
            {
                RakNet::BitStream bs(packet->data, packet->length, false);
                bs.IgnoreBytes(1);
                size_t length = 0;
                bs.Read(length);
                for(int i = 0; i < length; i++)
                {
                    RakNet::RakString str;
                    bs.Read(str);
                    data.players.push_back(str.C_String());
                }
                bs.Read(length);
                for(int i = 0; i < length; i++)
                {
                    RakNet::RakString str;
                    bs.Read(str);
                    data.plugins.push_back(str.C_String());
                }
                done = true;
            }
        }
    }

    peer->Shutdown(0);
    RakSleep(10);
    RakNet::RakPeerInterface::DestroyInstance(peer);
    return data;
}
