//
// Created by koncord on 13.05.17.
//

#ifndef NEWRESTAPI_RESTSERVER_HPP
#define NEWRESTAPI_RESTSERVER_HPP

#include <string>
#include <unordered_map>
#include "MasterServer.hpp"
#include "SimpleWeb/http_server.hpp"

typedef SimpleWeb::Server<SimpleWeb::HTTP> HttpServer;

class RestServer
{
public:
    RestServer(unsigned short port, MasterServer::ServerMap *pMap);
    void start();
    void stop();
    void cacheUpdated();

private:
    HttpServer httpServer;
    MasterServer::ServerMap *serverMap;
    bool updatedCache = true;
};


#endif //NEWRESTAPI_RESTSERVER_HPP
