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

#include "Networking.hpp"
#include "Main.hpp"
#include "GUIController.hpp"
#include "CellController.hpp"
#include "MechanicsHelper.hpp"
#include "LocalPlayer.hpp"
#include "DedicatedPlayer.hpp"

using namespace std;
using namespace mwmp;

Networking::Networking(): peer(RakNet::RakPeerInterface::GetInstance()), playerPacketController(peer),
    actorPacketController(peer), worldPacketController(peer)
{

    RakNet::SocketDescriptor sd;
    sd.port=0;
    RakNet::StartupResult b = peer->Startup(1,&sd, 1);
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
                    PlayerProcessor::SetServerAddr(packet->systemAddress);
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
            unsigned int crc32 = Utils::crc32checksum(col.getPath(*it).string());
            checksums.push_back(make_pair(*it, crc32));

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


    bool done = false;
    PacketPreInit::PluginContainer checksumsResponse;
    /*while (!done)
    {
        for (RakNet::Packet *packet = peer->Receive(); packet; peer->DeallocatePacket(packet), packet = peer->Receive())
        {
            if(packet->data[0] == ID_GAME_PREINIT)
            {
                RakNet::BitStream bsIn(&packet->data[0], packet->length, false);
                packetPreInit.Packet(&bsIn, guid, false, checksumsResponse);
                done = true;
            }
        }
    }*/

    if(!checksumsResponse.empty()) // something wrong
    {
        errmsg = "Your plugins\tShould be\n";
        for(int i = 0; i < checksumsResponse.size(); i++)
        {
            errmsg += checksums[i].first + " " + MyGUI::utility::toString(checksums[i].second) + "\t";
            errmsg += checksumsResponse[i].first + " " + MyGUI::utility::toString(checksumsResponse[i].second) + "\n";
        }
    }


    if (!errmsg.empty())
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_ERROR, errmsg.c_str());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "tes3mp", errmsg.c_str(), 0);
    }
}

void Networking::processPlayerPacket(RakNet::Packet *packet)
{
    if(!PlayerProcessor::Process(*packet))
        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Unhandled PlayerPacket with identifier %i has arrived", packet->data[0]);
}

void Networking::processActorPacket(RakNet::Packet *packet)
{
    RakNet::RakNetGUID guid;
    RakNet::BitStream bsIn(&packet->data[1], packet->length, false);
    bsIn.Read(guid);

    DedicatedPlayer *pl = 0;
    static RakNet::RakNetGUID myGuid = getLocalPlayer()->guid;
    if (guid != myGuid)
        pl = PlayerList::getPlayer(guid);

    ActorPacket *myPacket = actorPacketController.GetPacket(packet->data[0]);

    myPacket->setActorList(&actorList);
    myPacket->Packet(&bsIn, false);

    switch (packet->data[0])
    {
    case ID_ACTOR_LIST:
    {
        MWWorld::CellStore *ptrCellStore = Main::get().getCellController()->getCellStore(actorList.cell);

        if (!ptrCellStore) return;

        LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Received ID_ACTOR_LIST about %s", actorList.cell.getDescription().c_str());
        LOG_APPEND(Log::LOG_VERBOSE, "- action: %i", actorList.action);

        // If we've received a request for information, comply with it
        if (actorList.action == mwmp::BaseActorList::REQUEST)
            actorList.sendActorsInCell(ptrCellStore);

        break;
    }
    case ID_ACTOR_AUTHORITY:
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Received ID_ACTOR_AUTHORITY about %s", actorList.cell.getDescription().c_str());

        Main::get().getCellController()->initializeLocalActors(actorList.cell);
        Main::get().getCellController()->getCell(actorList.cell)->updateLocal(true);

        break;
    }
    case ID_ACTOR_POSITION:
    {
        //Main::get().getCellController()->readPositions(actorList);

        break;
    }
    case ID_ACTOR_ANIM_FLAGS:
    {
        //Main::get().getCellController()->readAnimFlags(actorList);

        break;
    }
    case ID_ACTOR_ANIM_PLAY:
    {
        //Main::get().getCellController()->readAnimPlay(actorList);

        break;
    }
    case ID_ACTOR_STATS_DYNAMIC:
    {
        //Main::get().getCellController()->readStatsDynamic(actorList);

        break;
    }
    case ID_ACTOR_SPEECH:
    {
        //Main::get().getCellController()->readSpeech(actorList);

        break;
    }
    case ID_ACTOR_ATTACK:
    {
        break;
    }
    case ID_ACTOR_CELL_CHANGE:
    {
        break;
    }
    case ID_ACTOR_TEST:
    {
        break;
    }
    default:
        LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Unhandled ActorPacket with identifier %i has arrived", packet->data[0]);
    }
}

void Networking::processWorldPacket(RakNet::Packet *packet)
{
    RakNet::RakNetGUID guid;
    RakNet::BitStream bsIn(&packet->data[1], packet->length, false);
    bsIn.Read(guid);

    DedicatedPlayer *pl = 0;
    static RakNet::RakNetGUID myGuid = getLocalPlayer()->guid;
    if (guid != myGuid)
        pl = PlayerList::getPlayer(guid);

    WorldPacket *myPacket = worldPacketController.GetPacket(packet->data[0]);

    myPacket->setEvent(&worldEvent);
    myPacket->Packet(&bsIn, false);

    switch (packet->data[0])
    {
    case ID_CONTAINER:
    {
        MWWorld::CellStore *ptrCellStore = Main::get().getCellController()->getCellStore(worldEvent.cell);

        if (!ptrCellStore) return;

        LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Received ID_CONTAINER about %s", worldEvent.cell.getDescription().c_str());
        LOG_APPEND(Log::LOG_VERBOSE, "- action: %i", worldEvent.action);

        // If we've received a request for information, comply with it
        if (worldEvent.action == mwmp::BaseEvent::REQUEST)
            worldEvent.sendContainers(ptrCellStore);
        // Otherwise, edit containers based on the information received
        else
            worldEvent.editContainers(ptrCellStore);

        break;
    }
    case ID_OBJECT_PLACE:
    {
        MWWorld::CellStore *ptrCellStore = Main::get().getCellController()->getCellStore(worldEvent.cell);

        if (!ptrCellStore) return;

        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_OBJECT_PLACE about %s", worldEvent.cell.getDescription().c_str());
        worldEvent.placeObjects(ptrCellStore);

        break;
    }
    case ID_OBJECT_DELETE:
    {
        MWWorld::CellStore *ptrCellStore = Main::get().getCellController()->getCellStore(worldEvent.cell);

        if (!ptrCellStore) return;

        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_OBJECT_DELETE about %s", worldEvent.cell.getDescription().c_str());
        worldEvent.deleteObjects(ptrCellStore);

        break;
    }
    case ID_OBJECT_LOCK:
    {
        MWWorld::CellStore *ptrCellStore = Main::get().getCellController()->getCellStore(worldEvent.cell);

        if (!ptrCellStore) return;

        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_OBJECT_LOCK about %s", worldEvent.cell.getDescription().c_str());
        worldEvent.lockObjects(ptrCellStore);

        break;
    }
    case ID_OBJECT_UNLOCK:
    {
        MWWorld::CellStore *ptrCellStore = Main::get().getCellController()->getCellStore(worldEvent.cell);

        if (!ptrCellStore) return;

        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_OBJECT_UNLOCK about %s", worldEvent.cell.getDescription().c_str());
        worldEvent.unlockObjects(ptrCellStore);

        break;
    }
    case ID_OBJECT_SCALE:
    {
        MWWorld::CellStore *ptrCellStore = Main::get().getCellController()->getCellStore(worldEvent.cell);

        if (!ptrCellStore) return;

        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_OBJECT_SCALE about %s", worldEvent.cell.getDescription().c_str());
        worldEvent.scaleObjects(ptrCellStore);

        break;
    }
    case ID_OBJECT_MOVE:
    {
        MWWorld::CellStore *ptrCellStore = Main::get().getCellController()->getCellStore(worldEvent.cell);

        if (!ptrCellStore) return;

        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_OBJECT_MOVE about %s", worldEvent.cell.getDescription().c_str());
        worldEvent.moveObjects(ptrCellStore);

        break;
    }
    case ID_OBJECT_ROTATE:
    {
        MWWorld::CellStore *ptrCellStore = Main::get().getCellController()->getCellStore(worldEvent.cell);

        if (!ptrCellStore) return;

        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_OBJECT_ROTATE about %s", worldEvent.cell.getDescription().c_str());
        worldEvent.rotateObjects(ptrCellStore);

        break;
    }
    case ID_OBJECT_ANIM_PLAY:
    {
        MWWorld::CellStore *ptrCellStore = Main::get().getCellController()->getCellStore(worldEvent.cell);

        if (!ptrCellStore) return;

        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_OBJECT_ANIM_PLAY about %s", worldEvent.cell.getDescription().c_str());
        worldEvent.animateObjects(ptrCellStore);

        break;
    }
    case ID_DOOR_STATE:
    {
        MWWorld::CellStore *ptrCellStore = Main::get().getCellController()->getCellStore(worldEvent.cell);

        if (!ptrCellStore) return;

        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_DOOR_STATE about %s", worldEvent.cell.getDescription().c_str());
        worldEvent.activateDoors(ptrCellStore);

        break;
    }
    case ID_SCRIPT_LOCAL_SHORT:
    {
        MWWorld::CellStore *ptrCellStore = Main::get().getCellController()->getCellStore(worldEvent.cell);

        if (!ptrCellStore) return;

        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_SCRIPT_LOCAL_SHORT about %s", worldEvent.cell.getDescription().c_str());
        worldEvent.setLocalShorts(ptrCellStore);

        break;
    }
    case ID_SCRIPT_LOCAL_FLOAT:
    {
        MWWorld::CellStore *ptrCellStore = Main::get().getCellController()->getCellStore(worldEvent.cell);

        if (!ptrCellStore) return;

        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_SCRIPT_LOCAL_FLOAT about %s", worldEvent.cell.getDescription().c_str());
        worldEvent.setLocalFloats(ptrCellStore);

        break;
    }
    case ID_SCRIPT_MEMBER_SHORT:
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_SCRIPT_MEMBER_SHORT");
        worldEvent.setMemberShorts();

        break;
    }
    case ID_SCRIPT_GLOBAL_SHORT:
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_SCRIPT_GLOBAL_SHORT");
        worldEvent.setGlobalShorts();

        break;
    }
    case ID_MUSIC_PLAY:
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_MUSIC_PLAY");
        worldEvent.playMusic();

        break;
    }
    case ID_VIDEO_PLAY:
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_VIDEO_PLAY");
        worldEvent.playVideo();

        break;
    }
    default:
        LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Unhandled WorldPacket with identifier %i has arrived", packet->data[0]);
    }
}

void Networking::receiveMessage(RakNet::Packet *packet)
{
    if (packet->length < 2)
        return;

    if (playerPacketController.ContainsPacket(packet->data[0]))
    {
        processPlayerPacket(packet);
    }
    else if (actorPacketController.ContainsPacket(packet->data[0]))
    {
        processActorPacket(packet);
    }
    else if (worldPacketController.ContainsPacket(packet->data[0]))
    {
        processWorldPacket(packet);
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

bool Networking::isDedicatedPlayer(const MWWorld::Ptr &ptr)
{
    if (ptr.mRef == 0)
        return 0;
    DedicatedPlayer *pl = PlayerList::getPlayer(ptr);

    return pl != 0;
}

bool Networking::attack(const MWWorld::Ptr &ptr)
{
    DedicatedPlayer *pl = PlayerList::getPlayer(ptr);

    if (pl == 0)
        return false;

    return pl->attack.pressed;
}

bool Networking::isConnected()
{
    return connected;
}
