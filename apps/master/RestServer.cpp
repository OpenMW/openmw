//
// Created by koncord on 13.05.17.
//

#include "RestServer.hpp"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using namespace std;
using namespace chrono;
using namespace boost::property_tree;

static string response201 = "HTTP/1.1 201 Created\r\nContent-Length: 7\r\n\r\nCreated";
static string response202 = "HTTP/1.1 202 Accepted\r\nContent-Length: 8\r\n\r\nAccepted";
static string response400 = "HTTP/1.1 400 Bad Request\r\nContent-Length: 11\r\n\r\nbad request";

inline void ResponseStr(HttpServer::Response &response, string content, string type = "", string code = "200 OK")
{
    response << "HTTP/1.1 " << code << "\r\n";
    if (!type.empty())
        response << "Content-Type: " << type <<"\r\n";
    response << "Content-Length: " << content.length() << "\r\n\r\n" << content;
}

inline void ptreeToServer(boost::property_tree::ptree &pt, MasterServer::SServer &server)
{
    server.SetName(pt.get<string>("hostname").c_str());
    server.SetGameMode(pt.get<string>("modname").c_str());
    server.SetVersion(pt.get<string>("version").c_str());
    server.SetPassword(pt.get<bool>("passw"));
    //server.query_port = pt.get<unsigned short>("query_port");
    server.SetPlayers(pt.get<unsigned>("players"));
    server.SetMaxPlayers(pt.get<unsigned>("max_players"));
}

inline void queryToStringStream(stringstream &ss, string addr, MasterServer::SServer &query)
{
    ss <<"\"" << addr << "\":{";
    ss << "\"modname\": \"" << query.GetGameMode() << "\"" << ", ";
    ss << "\"passw\": " << (query.GetPassword() ? "true" : "false") << ", ";
    ss << "\"hostname\": \"" << query.GetName() << "\"" << ", ";
    ss << "\"query_port\": " << 0 << ", ";
    ss << "\"last_update\": " << duration_cast<seconds>(steady_clock::now() - query.lastUpdate).count() << ", ";
    ss << "\"players\": " << query.GetPlayers() << ", ";
    ss << "\"version\": \"" << query.GetVersion() << "\"" << ", ";
    ss << "\"max_players\": " << query.GetMaxPlayers();
    ss << "}";
}

RestServer::RestServer(unsigned short port, MasterServer::ServerMap *pMap) : serverMap(pMap)
{
    httpServer.config.port = port;
}

void RestServer::start()
{
    static const string ValidIpAddressRegex = "(?:[0-9]{1,3}\\.){3}[0-9]{1,3}";
    static const string ValidPortRegex = "(?:[0-9]{1,4}|[1-5][0-9]{4}|6[0-4][0-9]{3}|65[0-4][0-9]{2}|655[0-2][0-9]|6553[0-5])$";
    static const string ServersRegex = "^/api/servers(?:/(" + ValidIpAddressRegex + "\\:" + ValidPortRegex + "))?";

    httpServer.resource[ServersRegex]["GET"] = [this](auto response, auto request) {
        if (request->path_match[1].length() > 0)
        {
            try
            {
                stringstream ss;
                ss << "{";
                auto addr = request->path_match[1].str();
                auto port = (unsigned short)stoi(&(addr[addr.find(':')+1]));
                queryToStringStream(ss, "server", serverMap->at(RakNet::SystemAddress(addr.c_str(), port)));
                ss << "}";
                ResponseStr(*response, ss.str(), "application/json");
            }
            catch(out_of_range e)
            {
                *response << response400;
            }
        }
        else
        {
            static string str;

            //if (updatedCache)
            {
                stringstream ss;
                ss << "{";
                ss << "\"list servers\":{";
                for (auto query = serverMap->begin(); query != serverMap->end(); query++)
                {
                    queryToStringStream(ss, query->first.ToString(true, ':'), query->second);
                    if (next(query) != serverMap->end())
                        ss << ", ";
                }
                ss << "}}";
                ResponseStr(*response, ss.str(), "application/json");
                updatedCache = false;
            }
            *response << str;
        }
    };

    //Add query for < 0.6 servers
    httpServer.resource[ServersRegex]["POST"] = [this](auto response, auto request) {
        try
        {
            ptree pt;
            read_json(request->content, pt);

            MasterServer::SServer server;
            ptreeToServer(pt, server);

            unsigned short port = pt.get<unsigned short>("port");
            server.lastUpdate = steady_clock::now();
            serverMap->insert({RakNet::SystemAddress(request->remote_endpoint_address.c_str(), port), server});
            updatedCache = true;

            *response << response201;
        }
        catch (exception& e)
        {
            cout << e.what() << endl;
            *response << response400;
        }
    };

    //Update query for < 0.6 servers
    httpServer.resource[ServersRegex]["PUT"] = [this](auto response, auto request) {
        auto addr = request->path_match[1].str();
        auto port = (unsigned short)stoi(&(addr[addr.find(':')+1]));

        auto query = serverMap->find(RakNet::SystemAddress(request->remote_endpoint_address.c_str(), port));

        if (query == serverMap->end())
        {
            cout << request->remote_endpoint_address + ": Trying to update a non-existent server or without permissions." << endl;
            *response << response400;
            return;
        }

        if (request->content.size() != 0)
        {
            try
            {
                ptree pt;
                read_json(request->content, pt);

                ptreeToServer(pt, query->second);

                updatedCache = true;
            }
            catch(exception &e)
            {
                cout << e.what() << endl;
                *response << response400;
            }
        }

        query->second.lastUpdate = steady_clock::now();

        *response << response202;
    };

    httpServer.resource["/api/servers/info"]["GET"] = [this](auto response, auto /*request*/) {
        stringstream ss;
        ss << '{';
        ss << "\"servers\": " << serverMap->size();
        unsigned int players = 0;
        for (auto s : *serverMap)
            players += s.second.GetPlayers();
        ss << ", \"players\": " << players;
        ss << "}";

        ResponseStr(*response, ss.str(), "application/json");
    };

    httpServer.default_resource["GET"]=[](auto response, auto /*request*/) {
        *response << response400;
    };

    httpServer.start();
}

void RestServer::cacheUpdated()
{
    updatedCache = true;
}

void RestServer::stop()
{
    httpServer.stop();
}
