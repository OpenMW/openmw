//
// Created by koncord on 07.01.17.
//

#include <RakPeer.h>
#include <HTTPConnection2.h>
#include <TCPInterface.h>
#include <RakSleep.h>

#include <sstream>

#include "HTTPNetwork.hpp"

using namespace RakNet;

HTTPNetwork::HTTPNetwork(std::string addr, unsigned short port) : address(addr), port(port)
{
    httpConnection = HTTPConnection2::GetInstance();
    tcpInterface = new TCPInterface;
    tcpInterface->Start(0, 64);
    tcpInterface->AttachPlugin(httpConnection);
}

HTTPNetwork::~HTTPNetwork()
{
    delete tcpInterface;
}

std::string HTTPNetwork::answer()
{
    RakNet::SystemAddress sa;
    RakNet::Packet *packet;
    RakNet::SystemAddress hostReceived;
    RakNet::RakString response;
    RakNet::RakString transmitted, hostTransmitted;
    int contentOffset = 0;

    while (true)
    {
        // This is kind of crappy, but for TCP plugins, always do HasCompletedConnectionAttempt,
        // then Receive(), then HasFailedConnectionAttempt(),HasLostConnection()
        sa = tcpInterface->HasCompletedConnectionAttempt();
        if (sa != RakNet::UNASSIGNED_SYSTEM_ADDRESS)
            printf("Connected to master server: %s\n", sa.ToString());

        sa = tcpInterface->HasFailedConnectionAttempt();
        if (sa != RakNet::UNASSIGNED_SYSTEM_ADDRESS)
        {
            printf("Failed to connect to master server: %s\n", sa.ToString());
            return "FAIL_CONNECT";
        }
        sa = tcpInterface->HasLostConnection();
        if (sa != RakNet::UNASSIGNED_SYSTEM_ADDRESS)
        {
            printf("Lost connection to master server: %s\n", sa.ToString());
            return "LOST_CONNECTION";
        }

        for (packet = tcpInterface->Receive(); packet; tcpInterface->DeallocatePacket(
                packet), packet = tcpInterface->Receive());

        if (httpConnection->GetResponse(transmitted, hostTransmitted, response, hostReceived, contentOffset))
        {
            if (contentOffset < 0)
                return "NO_CONTENT"; // no content
            tcpInterface->CloseConnection(sa);

            return (response.C_String() + contentOffset);
        }
        RakSleep(30);
    }
}

std::string HTTPNetwork::getData(const char *uri)
{
    RakNet::RakString createRequest = RakNet::RakString::FormatForGET(uri);

    if (!httpConnection->TransmitRequest(createRequest, address.c_str(), port))
        return "UNKNOWN_ADDRESS";
    return answer();
}

std::string HTTPNetwork::getDataPOST(const char *uri, const char* body, const char* contentType)
{
    RakNet::RakString createRequest = RakNet::RakString::FormatForPOST(uri, contentType, body);

    if (!httpConnection->TransmitRequest(createRequest, address.c_str(), port))
        return "UNKNOWN_ADDRESS";
    return answer();
}

std::string HTTPNetwork::getDataPUT(const char *uri, const char* body, const char* contentType)
{
    RakNet::RakString createRequest = RakNet::RakString::FormatForPUT(uri, contentType, body);

    if (!httpConnection->TransmitRequest(createRequest, address.c_str(), port))
        return "UNKNOWN_ADDRESS";
    return answer();
}
