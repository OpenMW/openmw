//
// Created by koncord on 07.01.17.
//

#ifndef NEWLAUNCHER_PING_HPP
#define NEWLAUNCHER_PING_HPP

#include <vector>
#include <string>


#define PING_UNREACHABLE 999

unsigned int PingRakNetServer(const char *addr, unsigned short port);

struct ServerExtendedData
{
    std::vector<std::string> players;
    std::vector<std::string> plugins;
};

ServerExtendedData getExtendedData(const char *addr, unsigned short port);

#endif //NEWLAUNCHER_PING_HPP
