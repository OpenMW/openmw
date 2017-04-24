//
// Created by koncord on 21.04.17.
//

#ifndef NEWMASTERPROTO_MASTERDATA_HPP
#define NEWMASTERPROTO_MASTERDATA_HPP

#include <string>
#include <vector>
#include <map>
#include <list>
#include <MessageIdentifiers.h>

enum MASTER_PACKETS
{
    ID_MASTER_QUERY = ID_USER_PACKET_ENUM,
    ID_MASTER_UPDATE,
    ID_MASTER_ANNOUNCE
};

struct ServerRule
{
    char type;

    std::string str;
    double val;
};

struct Plugin
{
    std::string name;
    unsigned long hash;
};

struct QueryData
{
    QueryData()
    {
        rules["name"].type = 's';
        rules["name"].str = "";
        rules["version"].type = 's';
        rules["version"].str = "";
        rules["players"].type = 'v';
        rules["players"].val = 0;
        rules["maxPlayers"].type = 'v';
        rules["maxPlayers"].val = 0;
        rules["gamemode"].type = 's';
        rules["gamemode"].str = "";
        rules["passw"].type = 'v';
        rules["passw"].val = 0;
    }
    const char *GetName() { return rules["name"].str.c_str(); }
    void SetName(const char *name) { rules["name"].str = name; }

    const char *GetVersion() { return rules["version"].str.c_str(); }
    void SetVersion(const char *version) { rules["version"].str = version; }

    int GetPlayers() { return rules["players"].val; }
    void SetPlayers(int players) { rules["players"].val = players; }

    int GetMaxPlayers() { return rules["maxPlayers"].val; }
    void SetMaxPlayers(int players) { rules["maxPlayers"].val = players; }

    const char *GetGameMode() { return rules["gamemode"].str.c_str(); }
    void SetGameMode(const char *str) { rules["gamemode"].str = str; }

    void SetPassword(int value) { rules["passw"].val = value; };
    int GetPassword() { return rules["passw"].val; }


    std::list<std::string> players;
    std::map<std::string, ServerRule> rules;
    std::vector<Plugin> plugins;
};

#endif //NEWMASTERPROTO_MASTERDATA_HPP
