//
// Created by koncord on 14.08.16.
//

#ifndef OPENMW_MASTERCLIENT_HPP
#define OPENMW_MASTERCLIENT_HPP

#include <string>
#include <mutex>
#include <thread>
#include <components/openmw-mp/Master/MasterData.hpp>
#include <RakString.h>
#include <components/openmw-mp/Master/PacketMasterAnnounce.hpp>

class MasterClient
{
public:
    static const unsigned int step_rate = 1000;
    static const unsigned int min_rate = 1000;
    static const unsigned int max_rate = 60000;
public:
    MasterClient(RakNet::RakPeerInterface *peer, std::string queryAddr, unsigned short queryPort);
    void SetPlayers(unsigned pl);
    void SetMaxPlayers(unsigned pl);
    void SetHostname(std::string hostname);
    void SetModname(std::string hostname);
    void SetRuleString(std::string key, std::string value);
    void SetRuleValue(std::string key, double value);
    void PushPlugin(Plugin plugin);

    bool Process(RakNet::Packet *packet);
    void Start();
    void Stop();
    void SetUpdateRate(unsigned int rate);

private:
    void Send(mwmp::PacketMasterAnnounce::Func func);
    void Thread();
private:
    RakNet::SystemAddress masterServer;
    RakNet::RakPeerInterface *peer;
    QueryData queryData;
    unsigned int timeout;
    static bool sRun;
    std::mutex mutexData;
    std::thread thrQuery;
    mwmp::PacketMasterAnnounce pma;
    RakNet::BitStream writeStream;
    bool updated;
};


#endif //OPENMW_MASTERCLIENT_HPP
