//
// Created by koncord on 14.08.16.
//

#ifndef OPENMW_MASTERCLIENT_HPP
#define OPENMW_MASTERCLIENT_HPP

#include <string>
#include <HTTPConnection2.h>
#include <TCPInterface.h>
#include <mutex>
#include <thread>

class MasterClient
{
public:
    static const unsigned int step_rate = 1000;
    static const unsigned int min_rate = 1000;
    static const unsigned int max_rate = 30000;
public:
    MasterClient(std::string queryAddr, unsigned short queryPort, std::string serverAddr, unsigned short serverPort);
    void SetPlayers(unsigned pl);
    void SetMaxPlayers(unsigned pl);
    void SetHostname(std::string hostname);
    void SetModname(std::string hostname);
    void Update();
    void Start();
    void Stop();
    void SetUpdateRate(unsigned int rate);

private:
    RakNet::RakString
    Send(std::string hostname, std::string modname, unsigned maxPlayers, bool update, unsigned players);
private:
    std::string queryAddr;
    unsigned short queryPort;
    std::string serverAddr;
    unsigned short serverPort;
    std::string hostname;
    std::string modname;
    unsigned players, maxPlayers;
    RakNet::HTTPConnection2 *httpConnection;
    RakNet::TCPInterface tcpInterface;
    unsigned int timeout;
    static bool sRun;
    std::mutex mutexData;
    std::thread thrQuery;

};


#endif //OPENMW_MASTERCLIENT_HPP
