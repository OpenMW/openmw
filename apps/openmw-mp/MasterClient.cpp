//
// Created by koncord on 14.08.16.
//

#include <RakSleep.h>
#include <Getche.h>
#include <sstream>
#include <iostream>
#include <thread>
#include <RakPeerInterface.h>
#include "MasterClient.hpp"
#include <components/openmw-mp/Log.hpp>
#include <components/openmw-mp/Version.hpp>
#include <components/openmw-mp/Master/PacketMasterAnnounce.hpp>
#include "Networking.hpp"

using namespace std;
using namespace mwmp;
using namespace RakNet;

bool MasterClient::sRun = false;

MasterClient::MasterClient(RakNet::RakPeerInterface *peer, std::string queryAddr, unsigned short queryPort) :
        masterServer(queryAddr.c_str(), queryPort), peer(peer), pma(peer)
{
    timeout = 15000; // every 15 seconds
    pma.SetSendStream(&writeStream);
    pma.SetServer(&queryData);
    updated = true;
}

void MasterClient::SetPlayers(unsigned pl)
{
    mutexData.lock();
    if (queryData.GetPlayers() != pl)
    {
        queryData.SetPlayers(pl);
        updated = true;
    }
    mutexData.unlock();
}

void MasterClient::SetMaxPlayers(unsigned pl)
{
    mutexData.lock();
    if (queryData.GetMaxPlayers() != pl)
    {
        queryData.SetMaxPlayers(pl);
        updated = true;
    }
    mutexData.unlock();
}

void MasterClient::SetHostname(std::string hostname)
{
    mutexData.lock();
    string substr = hostname.substr(0, 200);
    if (queryData.GetName() != substr)
    {
        queryData.SetName(substr.c_str());
        updated = true;
    }
    mutexData.unlock();
}

void MasterClient::SetModname(std::string modname)
{
    mutexData.lock();
    string substr = modname.substr(0, 200);
    if (queryData.GetGameMode() != substr)
    {
        queryData.SetGameMode(substr.c_str());
        updated = true;
    }
    mutexData.unlock();
}

void MasterClient::SetRuleString(std::string key, std::string value)
{
    mutexData.lock();
    if (queryData.rules.find(key) == queryData.rules.end() || queryData.rules[key].type != 's'
        || queryData.rules[key].str != value)
    {
        ServerRule rule;
        rule.str = value;
        rule.type = 's';
        queryData.rules.insert({key, rule});
        updated = true;
    }
    mutexData.unlock();
}

void MasterClient::SetRuleValue(std::string key, double value)
{
    mutexData.lock();
    if (queryData.rules.find(key) == queryData.rules.end() || queryData.rules[key].type != 'v'
        || queryData.rules[key].val != value)
    {
        ServerRule rule;
        rule.val = value;
        rule.type = 'v';
        queryData.rules.insert({key, rule});
        updated = true;
    }
    mutexData.unlock();
}

void MasterClient::PushPlugin(Plugin plugin)
{
    mutexData.lock();
    queryData.plugins.push_back(plugin);
    updated = true;
    mutexData.unlock();
}

bool MasterClient::Process(RakNet::Packet *packet)
{
    if (!sRun || packet->systemAddress != masterServer)
        return false;

    BitStream rs(packet->data, packet->length, false);
    unsigned char pid;
    rs.Read(pid);
    switch (pid)
    {
        case ID_CONNECTION_ATTEMPT_FAILED:
        case ID_CONNECTION_REQUEST_ACCEPTED:
        case ID_DISCONNECTION_NOTIFICATION:
            break;
        case ID_MASTER_QUERY:
            break;
        case ID_MASTER_ANNOUNCE:
            pma.SetReadStream(&rs);
            pma.Read();
            if (pma.GetFunc() == PacketMasterAnnounce::FUNCTION_KEEP)
                LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Server data successfully updated on master server");
            else if (pma.GetFunc() == PacketMasterAnnounce::FUNCTION_DELETE)
            {
                if (timeout != 0)
                {
                    LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Update rate is too low,"
                            " and the master server has deleted information about the server. Trying low rate...");
                    if ((timeout - step_rate) >= step_rate)
                        SetUpdateRate(timeout - step_rate);
                    updated = true;
                }
            }
            break;
        default:
            LOG_MESSAGE_SIMPLE(Log::LOG_ERROR, "Received wrong packet from master server with id: %d", packet->data[0]);
            return false;
    }
    return true;
}

void MasterClient::Send(mwmp::PacketMasterAnnounce::Func func)
{
    peer->Connect(masterServer.ToString(false), masterServer.GetPort(), TES3MP_MASTERSERVER_PASSW,
                  strlen(TES3MP_MASTERSERVER_PASSW), 0, 0, 5, 500);
    bool waitForConnect = true;
    while (waitForConnect)
    {
        ConnectionState state = peer->GetConnectionState(masterServer);
        switch (state)
        {
            case IS_CONNECTED:
                waitForConnect = false;
                break;
            case IS_NOT_CONNECTED:
            case IS_DISCONNECTED:
            case IS_SILENTLY_DISCONNECTING:
            case IS_DISCONNECTING:
            {
                LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Cannot connect to master server: %s", masterServer.ToString());
                return;
            }
            case IS_PENDING:
            case IS_CONNECTING:
                break;
        }
        RakSleep(500);
    }
    pma.SetFunc(func);
    pma.Send(masterServer);
    updated = false;
}

void MasterClient::Thread()
{
    assert(!sRun);

    sRun = true;

    queryData.SetPassword((int) Networking::get().isPassworded());
    queryData.SetVersion(TES3MP_VERSION);

    auto *players = Players::getPlayers();
    while (sRun)
    {
        SetPlayers((int) players->size());

        auto pIt = players->begin();
        if (queryData.players.size() != players->size())
        {
            queryData.players.clear();
            updated = true;
        }
        else
        {
            for (int i = 0; pIt != players->end(); i++, pIt++)
            {
                if (queryData.players[i] != pIt->second->npc.mName)
                {
                    queryData.players.clear();
                    updated = true;
                    break;
                }
            }
        }

        if (updated)
        {
            updated = false;
            if (pIt != players->end())
            {
                for (auto player : *players)
                {
                    if (!player.second->npc.mName.empty())
                        queryData.players.push_back(player.second->npc.mName);
                }
            }
            Send(PacketMasterAnnounce::FUNCTION_ANNOUNCE);
        }
        else
            Send(PacketMasterAnnounce::FUNCTION_KEEP);
        RakSleep(timeout);
    }
}

void MasterClient::Start()
{
    thrQuery = thread(&MasterClient::Thread, this);
}

void MasterClient::Stop()
{
    if (!sRun)
        return;
    sRun = false;
    if (thrQuery.joinable())
        thrQuery.join();
}

void MasterClient::SetUpdateRate(unsigned int rate)
{
    if (timeout < min_rate)
        timeout = min_rate;
    else if (timeout > max_rate)
        timeout = max_rate;
    timeout = rate;
}
