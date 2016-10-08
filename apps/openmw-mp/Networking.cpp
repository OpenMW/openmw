//
// Created by koncord on 12.01.16.
//

#include "Player.hpp"
#include <RakPeer.h>
#include <Kbhit.h>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Log.hpp>
#include <iostream>
#include <Script/Script.hpp>
#include <Script/API/TimerAPI.hpp>
#include <chrono>
#include <thread>

#include "Networking.hpp"

using namespace mwmp;
using namespace std;

Networking *Networking::sThis = 0;

Networking::Networking(RakNet::RakPeerInterface *peer)
{
    sThis = this;
    this->peer = peer;
    players = Players::GetPlayers();

    controller = new PacketsController(peer);

    controller->SetStream(0, &bsOut); // set send stream

    running = true;
    exitCode = 0;

    Script::Call<Script::CallbackIdentity("OnServerInit")>();
}

Networking::~Networking()
{
    Script::Call<Script::CallbackIdentity("OnServerExit")>(false);

    sThis = 0;
    delete controller;
    LOG_QUIT();
}

void Networking::Update(RakNet::Packet *packet)
{

    Player *player = Players::GetPlayer(packet->guid);

    RakNet::BitStream bsIn(&packet->data[1], packet->length, false);

    {
        RakNet::RakNetGUID ignoredGUID;
        bsIn.Read(ignoredGUID);
        (void)ignoredGUID;
    }

    controller->SetStream(&bsIn, 0);

    if (player == 0)
    {
        controller->GetPacket(ID_HANDSHAKE)->RequestData(packet->guid);
        Players::NewPlayer(packet->guid);
        player = Players::GetPlayer(packet->guid);
        controller->GetPacket(ID_USER_MYID)->Send(Players::GetPlayer(packet->guid), false);
        return;
    }

    BasePacket *myPacket = controller->GetPacket(packet->data[0]);

    if (packet->data[0] == ID_HANDSHAKE)
    {
        DEBUG_PRINTF("ID_HANDSHAKE\n");
        string passw = "SuperPassword";

        myPacket->Read(player);

        if (player->isHandshaked())
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Wrong handshake with player %d, name: %s",
                player->GetID(),
                player->Npc()->mName.c_str());
            KickPlayer(player->guid);
            return;
        }

        if (*player->GetPassw() != passw)
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Wrong server password for player %d, name: %s (pass: %s)",
                player->GetID(),
                player->Npc()->mName.c_str(),
                player->GetPassw()->c_str());
            KickPlayer(player->guid);
            return;
        }
        player->Handshake();
        return;
    }

    if (!player->isHandshaked())
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Have not completed handshake with player %d",
            player->GetID());
        //KickPlayer(player->guid);
        return;
    }

    if (packet->data[0] == ID_LOADED)
    {
        player->Loaded(Player::LOADED);

        static constexpr unsigned int ident = Script::CallbackIdentity("OnPlayerConnect");
        Script::CallBackReturn<ident> result = true;
        Script::Call<ident>(result, Players::GetPlayer(packet->guid)->GetID());

        if (!result)
        {
            controller->GetPacket(ID_USER_DISCONNECTED)->Send(Players::GetPlayer(packet->guid), false);
            Players::DeletePlayer(packet->guid);
            return;
        }
    }
    else if (packet->data[0] == ID_GAME_BASE_INFO)
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Received ID_GAME_BASE_INFO about %s",
                           player->Npc()->mName.c_str());

        myPacket->Read(player);
        myPacket->Send(player, true);
    }

    if (player->LoadedState() == Player::NOTLOADED)
        return;
    else if (player->LoadedState() == Player::LOADED)
    {
        player->Loaded(Player::POSTLOADED);
        NewPlayer(packet->guid);
        return;
    }

    switch (packet->data[0])
    {
        /*case ID_GAME_BASE_INFO:
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Received ID_GAME_BASE_INFO about %s",
                player->Npc()->mName.c_str());

            myPacket->Read(player);
            myPacket->Send(player, true);

            break;
        }*/
        case ID_GAME_POS:
        {
            //DEBUG_PRINTF("ID_GAME_POS \n");

            if (!player->CreatureStats()->mDead)
            {
                myPacket->Read(player);
                myPacket->Send(player, true); //send to other clients
            }

            break;
        }
        case ID_GAME_CELL:
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Received ID_GAME_CELL from %s",
                player->Npc()->mName.c_str());

            if (!player->CreatureStats()->mDead)
            {
                myPacket->Read(player);

                LOG_APPEND(Log::LOG_INFO, "- Moved to %s",
                    player->GetCell()->getDescription().c_str());

                myPacket->Send(player, true); //send to other clients
                Script::Call<Script::CallbackIdentity("OnPlayerChangeCell")>(player->GetID());
            }
            else
            {
                LOG_APPEND(Log::LOG_INFO, "- Ignored because %s is dead",
                    player->Npc()->mName.c_str());
            }

            break;
        }
        case ID_GAME_ATTRIBUTE:
        {

            if (!player->CreatureStats()->mDead)
            {
                myPacket->Read(player);
                myPacket->Send(player, true);

                Script::Call<Script::CallbackIdentity("OnPlayerChangeAttributes")>(player->GetID());
            }

            break;
        }
        case ID_GAME_SKILL:
        {

            if (!player->CreatureStats()->mDead)
            {
                myPacket->Read(player);
                myPacket->Send(player, true);

                Script::Call<Script::CallbackIdentity("OnPlayerChangeSkills")>(player->GetID());
            }

            break;
        }
        case ID_GAME_LEVEL:
        {

            if (!player->CreatureStats()->mDead)
            {
                myPacket->Read(player);
                myPacket->Send(player, true);

                Script::Call<Script::CallbackIdentity("OnPlayerChangeLevel")>(player->GetID());
            }

            break;
        }
        case ID_GAME_EQUIPMENT:
        {
            DEBUG_PRINTF("ID_GAME_EQUIPMENT\n");

            myPacket->Read(player);
            myPacket->Send(player, true);

            Script::Call<Script::CallbackIdentity("OnPlayerChangeEquipment")>(player->GetID());

            break;
        }

        case ID_GAME_ATTACK:
        {
            DEBUG_PRINTF("ID_GAME_ATTACK\n");

            if (!player->CreatureStats()->mDead)
            {
                myPacket->Read(player);

                Player *target = Players::GetPlayer(player->GetAttack()->target);

                if (target == nullptr)
                    target = player;

                LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Player: %s attaked %s state: %d", player->Npc()->mName.c_str(),
                                   target->Npc()->mName.c_str(), player->GetAttack()->pressed == 1);
                if (player->GetAttack()->pressed == 0)
                {
                    LOG_APPEND(Log::LOG_VERBOSE, "success: %d", player->GetAttack()->success == 1);
                    if (player->GetAttack()->success == 1)
                    {
                        LOG_APPEND(Log::LOG_VERBOSE, "damage: %d", player->GetAttack()->damage == 1);
                        player->setLastAttackerID(target->GetID());
                    }
                }

                myPacket->Send(player, true);
                controller->GetPacket(ID_GAME_DYNAMICSTATS)->RequestData(player->GetAttack()->target);
            }
            break;
        }

        case ID_GAME_DYNAMICSTATS:
        {
            DEBUG_PRINTF("ID_GAME_DYNAMICSTATS\n");
            myPacket->Read(player);
            myPacket->Send(player, true);
            break;
        }

        case ID_GAME_DIE:
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Received ID_GAME_DIE from %s",
                player->Npc()->mName.c_str());

            Player *killer = Players::GetPlayer(player->getLastAttackerID());

            short reason = 0; // unknown;

            if (!killer)
                killer = player;
            else if(killer->GetID() == player->GetID())
                reason = 2; //suicide
            else
                reason = 1; //killed

            player->resetLastAttacker();

            player->CreatureStats()->mDead = true;
            myPacket->Send(player, true);

            Script::Call<Script::CallbackIdentity("OnPlayerDeath")>(player->GetID(), reason, killer->GetID());

            break;
        }

        case ID_GAME_RESURRECT:
        {
            DEBUG_PRINTF("ID_GAME_RESURRECT\n");
            //packetResurrect.Read(player);
            player->CreatureStats()->mDead = false;
            myPacket->Send(player, true);
            controller->GetPacket(ID_GAME_POS)->RequestData(player->guid);
            controller->GetPacket(ID_GAME_CELL)->RequestData(player->guid);

            Script::Call<Script::CallbackIdentity("OnPlayerResurrect")>(player->GetID());

            break;
        }

        case ID_GAME_DRAWSTATE:
        {
            DEBUG_PRINTF("ID_GAME_DRAWSTATE\n");
            myPacket->Read(player);
            myPacket->Send(player, true);
            break;
        }

        case ID_CHAT_MESSAGE:
        {
            DEBUG_PRINTF("ID_CHAT_MESSAGE\n");
            myPacket->Read(player);
            Script::CallBackReturn<Script::CallbackIdentity("OnPlayerSendMessage")> result = true;
            Script::Call<Script::CallbackIdentity("OnPlayerSendMessage")>(result, player->GetID(), player->ChatMessage()->c_str());

            if (result)
            {
                *player->ChatMessage() = player->Npc()->mName + " (" + std::to_string(player->GetID()) + "): "
                                         + *player->ChatMessage() + "\n";
                myPacket->Send(player, false);
                myPacket->Send(player, true);
            }
            break;
        }
        case ID_GAME_CHARGEN:
        {
            DEBUG_PRINTF("ID_GAME_CHARGEN\n");
            myPacket->Read(player);

            if (player->CharGenStage()->current == player->CharGenStage()->end && player->CharGenStage()->current != 0)
            {
                Script::Call<Script::CallbackIdentity("OnPlayerEndCharGen")>(player->GetID());
                cout << "RACE: " << player->Npc()->mRace << endl;
            }
            break;
        }

        case ID_GUI_MESSAGEBOX:
        {
            DEBUG_PRINTF("ID_GUI_MESSAGEBOX\n");
            myPacket->Read(player);

            Script::Call<Script::CallbackIdentity("OnGUIAction")>(player->GetID(), (int)player->guiMessageBox.id,
                                                                  player->guiMessageBox.data.c_str());
            break;
        }

        case ID_GAME_CHARCLASS:
        {
            DEBUG_PRINTF("ID_GAME_CHARCLASS\n");
            myPacket->Read(player);
            break;
        }

        default:
            printf("Message with identifier %i has arrived.\n", packet->data[0]);
            break;
    }
}

void Networking::NewPlayer(RakNet::RakNetGUID guid)
{
    controller->GetPacket(ID_GAME_BASE_INFO)->RequestData(guid);
    controller->GetPacket(ID_GAME_DYNAMICSTATS)->RequestData(guid);
    controller->GetPacket(ID_GAME_POS)->RequestData(guid);
    controller->GetPacket(ID_GAME_CELL)->RequestData(guid);
    controller->GetPacket(ID_GAME_EQUIPMENT)->RequestData(guid);

    for (TPlayers::iterator pl = players->begin(); pl != players->end(); pl++) //sending other players to new player
    {
        if (pl->first == guid) continue;

        controller->GetPacket(ID_GAME_BASE_INFO)->Send(pl->second, guid);
        controller->GetPacket(ID_GAME_DYNAMICSTATS)->Send(pl->second, guid);
        controller->GetPacket(ID_GAME_ATTRIBUTE)->Send(pl->second, guid);
        controller->GetPacket(ID_GAME_SKILL)->Send(pl->second, guid);
        controller->GetPacket(ID_GAME_POS)->Send(pl->second, guid);
        controller->GetPacket(ID_GAME_CELL)->Send(pl->second, guid);
        controller->GetPacket(ID_GAME_EQUIPMENT)->Send(pl->second, guid);
    }

}



void Networking::DisconnectPlayer(RakNet::RakNetGUID guid)
{
    Script::Call<Script::CallbackIdentity("OnPlayerDisconnect")>(Players::GetPlayer(guid)->GetID());
    controller->GetPacket(ID_USER_DISCONNECTED)->Send(Players::GetPlayer(guid), true);
    Players::DeletePlayer(guid);
}

PacketsController *Networking::GetController() const
{
    return controller;
}

const Networking &Networking::Get()
{
    return *sThis;
}


Networking *Networking::GetPtr()
{
    return sThis;
}

void Networking::StopServer(int code)
{
    running = false;
    exitCode = code;
}

int Networking::MainLoop()
{
    RakNet::Packet *packet;

    while (running)
    {
        if (kbhit() && getch() == '\n')
            break;
        for (packet=peer->Receive(); packet; peer->DeallocatePacket(packet), packet=peer->Receive())
        {
            switch (packet->data[0])
            {
                case ID_REMOTE_DISCONNECTION_NOTIFICATION:
                    printf("Another client has disconnected.\n");
                    break;
                case ID_REMOTE_CONNECTION_LOST:
                    printf("Another client has lost connection.\n");
                    break;
                case ID_REMOTE_NEW_INCOMING_CONNECTION:
                    printf("Another client has connected.\n");
                    break;
                case ID_CONNECTION_REQUEST_ACCEPTED:    // client to server
                {
                    printf("Our connection request has been accepted.\n");
                    break;
                }
                case ID_NEW_INCOMING_CONNECTION:
                    printf("A connection is incoming.\n");
                    break;
                case ID_NO_FREE_INCOMING_CONNECTIONS:
                    printf("The server is full.\n");
                    break;
                case ID_DISCONNECTION_NOTIFICATION:
                    printf("A client has disconnected.\n");
                    DisconnectPlayer(packet->guid);
                    break;
                case ID_CONNECTION_LOST:
                    printf("A client has lost connection.\n");
                    DisconnectPlayer(packet->guid);
                    break;
                default:
                    Update(packet);
                    break;
            }
        }
        TimerAPI::Tick();
        this_thread::sleep_for(chrono::milliseconds(1));
    }

    TimerAPI::Terminate();
    return exitCode;
}

void Networking::KickPlayer(RakNet::RakNetGUID guid)
{
    peer->CloseConnection(guid, true);
}

unsigned short Networking::NumberOfConnections() const
{
    return peer->NumberOfConnections();
}

unsigned int Networking::MaxConnections() const
{
    return peer->GetMaximumIncomingConnections();
}