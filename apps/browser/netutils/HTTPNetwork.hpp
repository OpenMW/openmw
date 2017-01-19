//
// Created by koncord on 07.01.17.
//

#ifndef NEWLAUNCHER_HTTPNETWORK_HPP
#define NEWLAUNCHER_HTTPNETWORK_HPP


#include <string>

namespace RakNet
{
    class TCPInterface;
    class HTTPConnection2;
}

class HTTPNetwork
{
public:
    HTTPNetwork(std::string addr, unsigned short port);
    ~HTTPNetwork();
    std::string getData(const char *uri);
    std::string getDataPOST(const char *uri, const char* body,  const char* contentType = "application/json");
    std::string getDataPUT(const char *uri, const char* body, const char* contentType = "application/json");

protected:
    RakNet::TCPInterface *tcpInterface;
    RakNet::HTTPConnection2 *httpConnection;
    std::string address;
    unsigned short port;
    std::string answer();
};


#endif //NEWLAUNCHER_HTTPNETWORK_HPP
