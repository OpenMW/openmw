//
// Created by koncord on 14.08.16.
//

#ifndef OPENMW_MASTERCLIENT_HPP
#define OPENMW_MASTERCLIENT_HPP

#include <string>
#include <HTTPConnection2.h>
#include <TCPInterface.h>
#include <mutex>

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
    void SetMOTD(std::string &motd);
    void Update();
    void Stop();
    void SetUpdateRate(unsigned int rate);

private:
    RakNet::RakString Send(std::string motd, unsigned players, unsigned maxPlayers, bool update = true);
private:
    std::string queryAddr;
    unsigned short queryPort;
    std::string serverAddr;
    unsigned short serverPort;
    std::string motd;
    unsigned players, maxPlayers;
    RakNet::HTTPConnection2 *httpConnection;
    RakNet::TCPInterface tcpInterface;
    unsigned int timeout;
    static bool sRun;
    std::mutex mutexData;

};


#endif //OPENMW_MASTERCLIENT_HPP
