//
// Created by koncord on 24.04.17.
//

#ifndef OPENMW_QUERYCLIENT_HPP
#define OPENMW_QUERYCLIENT_HPP

#include <string>
#include <RakPeerInterface.h>
#include <components/openmw-mp/Master/PacketMasterQuery.hpp>
#include <components/openmw-mp/Master/PacketMasterUpdate.hpp>
#include <apps/browser/ServerModel.hpp>

class QueryClient
{
private:
    QueryClient(QueryClient const &) = delete;
    QueryClient(QueryClient &&) = delete;
    QueryClient &operator=(QueryClient const &) = delete;
    QueryClient &operator=(QueryClient &&) = delete;
public:
    static QueryClient &Get();
    void SetServer(std::string addr, unsigned short port);
    std::map<RakNet::SystemAddress, QueryData> Query();
    std::pair<RakNet::SystemAddress, QueryData> Update(RakNet::SystemAddress addr);
    int Status();
private:
    RakNet::ConnectionState Connect();
    MASTER_PACKETS GetAnswer();
protected:
    QueryClient();
    ~QueryClient();
private:
    int status;
    RakNet::RakPeerInterface *peer;
    RakNet::SystemAddress masterAddr;
    mwmp::PacketMasterQuery *pmq;
    mwmp::PacketMasterUpdate *pmu;
    std::pair<RakNet::SystemAddress, ServerData> server;

};


#endif //OPENMW_QUERYCLIENT_HPP
