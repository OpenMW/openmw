//
// Created by koncord on 04.01.16.
//

#include <stdexcept>
#include <iostream>
#include <string>

#include <components/openmw-mp/Log.hpp>
#include <components/openmw-mp/Utils.hpp>
#include <components/openmw-mp/Version.hpp>
#include <components/openmw-mp/Packets/PacketPreInit.hpp>

#include <components/esm/cellid.hpp>
#include <components/files/configurationmanager.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwclass/npc.hpp"

#include "../mwmechanics/combat.hpp"
#include "../mwmechanics/npcstats.hpp"

#include "../mwstate/statemanagerimp.hpp"

#include "../mwworld/cellstore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"

#include <SDL_messagebox.h>
#include <RakSleep.h>
#include <iomanip>

#include "Networking.hpp"
#include "Main.hpp"
#include "ProcessorInitializer.hpp"
#include "PlayerProcessor.hpp"
#include "WorldProcessor.hpp"
#include "ActorProcessor.hpp"
#include "GUIController.hpp"
#include "CellController.hpp"

using namespace std;
using namespace mwmp;

void printWithWidth(ostringstream &sstr, string str, size_t width)
{
    sstr << left << setw(width) << setfill(' ') << str;
}

string intToHexStr(unsigned val)
{
    ostringstream sstr;
    sstr << "0x" << setfill('0') << setw(8) << uppercase << hex << val;
    return sstr.str();
}

string comparePlugins(PacketPreInit::PluginContainer checksums, PacketPreInit::PluginContainer checksumsResponse,
                      bool full = false)
{
    std::ostringstream sstr;
    sstr << "Note: Use the same load order as the server.\n";
    size_t pluginNameLen1 = 0;
    size_t pluginNameLen2 = 0;
    for (size_t i = 0; i < checksums.size(); i++)
        if (pluginNameLen1 < checksums[i].first.size())
            pluginNameLen1 = checksums[i].first.size();

    for (size_t i = 0; i < checksumsResponse.size(); i++)
        if (pluginNameLen2 < checksumsResponse[i].first.size())
            pluginNameLen2 = checksumsResponse[i].first.size();

    printWithWidth(sstr, "Your current plugins are:", pluginNameLen1 + 16);
    sstr << "To join this server, use:\n";

    printWithWidth(sstr, "name", pluginNameLen1 + 2);
    printWithWidth(sstr, "hash", 14);
    printWithWidth(sstr, "name", pluginNameLen2 + 2);
    sstr << "hash\n";
    for (size_t i = 0; i < checksumsResponse.size(); i++)
    {
        string plugin;
        unsigned val;
        try
        {
            plugin = checksums.at(i).first;
            val = checksums.at(i).second[0];
        }
        catch(exception &e)
        {
            plugin = "null";
            val = 0;
        }

        if (plugin == "null")
        {
            printWithWidth(sstr, "", pluginNameLen1 + 16);
        }
        else
        {
            printWithWidth(sstr, plugin, pluginNameLen1 + 2);
            printWithWidth(sstr, intToHexStr(val), 14);
        }
        
        printWithWidth(sstr, checksumsResponse[i].first, pluginNameLen2 + 2);
        if (checksumsResponse[i].second.size() > 0)
        {
            if (full)
                for (size_t j = 0; j < checksumsResponse[i].second.size(); j++)
                    printWithWidth(sstr, intToHexStr(checksumsResponse[i].second[j]), 14);
            else
                sstr << intToHexStr(checksumsResponse[i].second[0]);
        }
        else
            sstr << "any";
        sstr << "\n";
    }
    sstr << "\nMore info in logs";
    return sstr.str();
}

Networking::Networking(): peer(RakNet::RakPeerInterface::GetInstance()), playerPacketController(peer),
    actorPacketController(peer), worldPacketController(peer)
{

    RakNet::SocketDescriptor sd;
    sd.port=0;
    peer->Startup(1, &sd, 1);
    RakAssert(b==RAKNET_STARTED);

    playerPacketController.SetStream(0, &bsOut);
    actorPacketController.SetStream(0, &bsOut);
    worldPacketController.SetStream(0, &bsOut);

    connected = 0;
    ProcessorInitializer();
}

Networking::~Networking()
{
    peer->Shutdown(100);
    peer->CloseConnection(peer->GetSystemAddressFromIndex(0), true, 0);
    RakNet::RakPeerInterface::DestroyInstance(peer);
}

void Networking::update()
{
    RakNet::Packet *packet;
    std::string errmsg = "";

    for (packet=peer->Receive(); packet; peer->DeallocatePacket(packet), packet=peer->Receive())
    {
        switch (packet->data[0])
        {
            case ID_REMOTE_DISCONNECTION_NOTIFICATION:
                LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Another client has disconnected.");
                break;
            case ID_REMOTE_CONNECTION_LOST:
                LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Another client has lost connection.");
                break;
            case ID_REMOTE_NEW_INCOMING_CONNECTION:
                LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Another client has connected.");
                break;
            case ID_CONNECTION_REQUEST_ACCEPTED:
                LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Our connection request has been accepted.");
                break;
            case ID_NEW_INCOMING_CONNECTION:
                LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "A connection is incoming.");
                break;
            case ID_NO_FREE_INCOMING_CONNECTIONS:
                errmsg = "The server is full.";
                break;
            case ID_DISCONNECTION_NOTIFICATION:
                errmsg = "We have been disconnected.";
                break;
            case ID_CONNECTION_LOST:
                errmsg = "Connection lost.";
                break;
            default:
                receiveMessage(packet);
                //LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Message with identifier %i has arrived.", packet->data[0]);
                break;
        }
    }

    if (!errmsg.empty())
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_ERROR, errmsg.c_str());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "tes3mp", errmsg.c_str(), 0);
        MWBase::Environment::get().getStateManager()->requestQuit();
    }
}

void Networking::connect(const std::string &ip, unsigned short port, std::vector<string> &content, Files::Collections &collections)
{
    RakNet::SystemAddress master;
    master.SetBinaryAddress(ip.c_str());
    master.SetPortHostOrder(port);
    std::string errmsg = "";

    stringstream sstr(TES3MP_VERSION);
    sstr << TES3MP_PROTO_VERSION;

    if (peer->Connect(master.ToString(false), master.GetPort(), sstr.str().c_str(), (int) sstr.str().size(), 0, 0, 3, 500, 0) != RakNet::CONNECTION_ATTEMPT_STARTED)
        errmsg = "Connection attempt failed.\n";

    bool queue = true;
    while (queue)
    {
        for (RakNet::Packet *packet = peer->Receive(); packet; peer->DeallocatePacket(
                packet), packet = peer->Receive())
        {
            switch (packet->data[0])
            {
                case ID_CONNECTION_ATTEMPT_FAILED:
                {
                    errmsg = "Connection failed.\n"
                            "Either the IP address is wrong or a firewall on either system is blocking\n"
                            "UDP packets on the port you have chosen.";
                    queue = false;
                    break;
                }
                case ID_INVALID_PASSWORD:
                {
                    errmsg = "Connection failed.\n"
                            "The client or server is outdated.";
                    queue = false;
                    break;
                }
                case ID_CONNECTION_REQUEST_ACCEPTED:
                {
                    serverAddr = packet->systemAddress;
                    BaseClientPacketProcessor::SetServerAddr(packet->systemAddress);

                    connected = true;
                    queue = false;

                    LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_CONNECTION_REQUESTED_ACCEPTED from %s",
                                       serverAddr.ToString());

                    break;
                }
                case ID_DISCONNECTION_NOTIFICATION:
                    throw runtime_error("ID_DISCONNECTION_NOTIFICATION.\n");
                case ID_CONNECTION_BANNED:
                    throw runtime_error("ID_CONNECTION_BANNED.\n");
                case ID_CONNECTION_LOST:
                    throw runtime_error("ID_CONNECTION_LOST.\n");
                default:
                    LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Connection message with identifier %i has arrived in initialization.",
                                       packet->data[0]);
            }
        }
    }

    if (!errmsg.empty())
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_ERROR, errmsg.c_str());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "tes3mp", errmsg.c_str(), 0);
    }
    else
        preInit(content, collections);
}

void Networking::preInit(std::vector<std::string> &content, Files::Collections &collections)
{
    std::string errmsg = "";
    PacketPreInit::PluginContainer checksums;
    vector<string>::const_iterator it(content.begin());
    for (int idx = 0; it != content.end(); ++it, ++idx)
    {
        boost::filesystem::path filename(*it);
        const Files::MultiDirCollection& col = collections.getCollection(filename.extension().string());
        if (col.doesExist(*it))
        {
            PacketPreInit::HashList hashList;
            unsigned crc32 = Utils::crc32checksum(col.getPath(*it).string());
            hashList.push_back(crc32);
            checksums.push_back(make_pair(*it, hashList));

            printf("idx: %d\tchecksum: %X\tfile: %s\n", idx, crc32, col.getPath(*it).string().c_str());
        }
        else
            throw std::runtime_error("Plugin doesn't exist.");
    }

    PacketPreInit packetPreInit(peer);
    RakNet::BitStream bs;
    RakNet::RakNetGUID guid;
    packetPreInit.setChecksums(&checksums);
    packetPreInit.setGUID(guid);
    packetPreInit.SetSendStream(&bs);
    packetPreInit.Send(serverAddr);

    PacketPreInit::PluginContainer checksumsResponse;
    bool done = false;
    while (!done)
    {
        RakNet::Packet *packet = peer->Receive();
        if (!packet)
        {
            RakSleep(500);
            continue;
        }

        RakNet::BitStream bsIn(&packet->data[0], packet->length, false);
        unsigned char packetId;
        bsIn.Read(packetId);
        switch(packetId)
        {
            case ID_DISCONNECTION_NOTIFICATION:
            case ID_CONNECTION_LOST:
                done = true;
                break;
            case ID_GAME_PREINIT:
                bsIn.IgnoreBytes((unsigned) RakNet::RakNetGUID::size());
                packetPreInit.setChecksums(&checksumsResponse);
                packetPreInit.Packet(&bsIn, false);
                done = true;
                break;
        }

        peer->DeallocatePacket(packet);
    }

    if (!checksumsResponse.empty()) // something wrong
        errmsg = comparePlugins(checksums, checksumsResponse);


    if (!errmsg.empty())
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_ERROR, comparePlugins(checksums, checksumsResponse, true).c_str());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "tes3mp", errmsg.c_str(), 0);
        MWBase::Environment::get().getStateManager()->requestQuit();
    }
}

void Networking::receiveMessage(RakNet::Packet *packet)
{
    if (packet->length < 2)
        return;

    if (playerPacketController.ContainsPacket(packet->data[0]))
    {
        if (!PlayerProcessor::Process(*packet))
            LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Unhandled PlayerPacket with identifier %i has arrived", packet->data[0]);
    }
    else if (actorPacketController.ContainsPacket(packet->data[0]))
    {
        if (!ActorProcessor::Process(*packet, actorList))
            LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Unhandled ActorPacket with identifier %i has arrived", packet->data[0]);
    }
    else if (worldPacketController.ContainsPacket(packet->data[0]))
    {
        if (!WorldProcessor::Process(*packet, worldEvent))
            LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Unhandled WorldPacket with identifier %i has arrived", packet->data[0]);
    }
}

PlayerPacket *Networking::getPlayerPacket(RakNet::MessageID id)
{
    return playerPacketController.GetPacket(id);
}

ActorPacket *Networking::getActorPacket(RakNet::MessageID id)
{
    return actorPacketController.GetPacket(id);
}

WorldPacket *Networking::getWorldPacket(RakNet::MessageID id)
{
    return worldPacketController.GetPacket(id);
}

LocalPlayer *Networking::getLocalPlayer()
{
    return mwmp::Main::get().getLocalPlayer();
}

ActorList *Networking::getActorList()
{
    return &actorList;
}

WorldEvent *Networking::getWorldEvent()
{
    return &worldEvent;
}

bool Networking::isConnected()
{
    return connected;
}
