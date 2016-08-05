//
// Created by koncord on 12.01.16.
//

#include "Player.hpp"
#include <RakPeer.h>
#include <Kbhit.h>
#include <components/openmw-mp/NetworkMessages.hpp>
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

    Script::Call<Script::CallbackIdentity("OnServerInit")>();

    running = true;
    exitCode = 0;
}

Networking::~Networking()
{
    Script::Call<Script::CallbackIdentity("OnServerExit")>(false);

    sThis = 0;
    delete controller;
}

void Networking::Update(RakNet::Packet *packet)
{

    Player *player = Players::GetPlayer(packet->guid);

    if(player == 0)
    {
        controller->GetPacket(ID_HANDSHAKE)->RequestData(packet->guid);

        NewPlayer(packet->guid);
        player = Players::GetPlayer(packet->guid);

        return;
    }

    RakNet::BitStream bsIn(&packet->data[1], packet->length, false);

    {
        RakNet::RakNetGUID ignoredGUID;
        bsIn.Read(ignoredGUID);
        (void)ignoredGUID;
    }

    controller->SetStream(&bsIn, 0);

    BasePacket *myPacket = controller->GetPacket(packet->data[0]);

    if(packet->data[0] == ID_HANDSHAKE)
    {
        DEBUG_PRINTF("ID_HANDSHAKE\n");
        string passw = "SuperPassword";

        myPacket->Read(player);

        if(player->isHandshaked())
        {
            DEBUG_PRINTF("Wrong handshake with player %d, name: %s\n", player->GetID(), player->Npc()->mName.c_str());
            KickPlayer(player->guid);
            return;
        }

        if(*player->GetPassw() != passw)
        {
            DEBUG_PRINTF("Wrong server password (player %d, name: %s) pass: %s\n", player->GetID(), player->Npc()->mName.c_str(), player->GetPassw()->c_str());
            KickPlayer(player->guid);
            return;
        }
        player->Handshake();

        static constexpr unsigned int ident = Script::CallbackIdentity("OnPlayerConnect");
        Script::CallBackReturn<ident> result = true;
        Script::Call<ident>(result, Players::GetPlayer(packet->guid)->GetID());

        if(!result)
        {
            controller->GetPacket(ID_USER_DISCONNECTED)->Send(Players::GetPlayer(packet->guid), false);
            Players::DeletePlayer(packet->guid);
            return;
        }

        return;

    }

    if(!player->isHandshaked())
    {
        DEBUG_PRINTF("Wrong auth player %d, name: %s\n", player->GetID(), player->Npc()->mName.c_str());
        //KickPlayer(player->guid);
        return;
    }

    switch(packet->data[0])
    {
        case ID_GAME_BASE_INFO:
        {
            DEBUG_PRINTF("ID_GAME_BASE_INFO\n");

            myPacket->Read(player);
            myPacket->Send(player, true);

            break;
        }
        case ID_GAME_UPDATE_POS:
        {
            //DEBUG_PRINTF("ID_GAME_UPDATE_POS \n");

            if(!player->CreatureStats()->mDead)
            {
                myPacket->Read(player);
                myPacket->Send(player, true); //send to other clients
            }

            break;
        }
        case ID_GAME_CELL:
        {
            DEBUG_PRINTF("ID_GAME_CELL \n");

            if(!player->CreatureStats()->mDead)
            {
                myPacket->Read(player);
                myPacket->Send(player, true); //send to other clients
                Script::Call<Script::CallbackIdentity("OnPlayerChangeCell")>(player->GetID());
            }

            break;
        }
        case ID_GAME_UPDATE_SKILLS:
        {
            DEBUG_PRINTF("ID_GAME_UPDATE_SKILLS\n");

            if(!player->CreatureStats()->mDead)
            {
                myPacket->Read(player);
                myPacket->Send(player, true);
            }

            break;
        }
        case ID_GAME_UPDATE_EQUIPED:
        {
            DEBUG_PRINTF("ID_GAME_UPDATE_EQUIPED\n");

            myPacket->Read(player);
            myPacket->Send(player, true);

            Script::Call<Script::CallbackIdentity("OnPlayerUpdateEquiped")>(player->GetID());

            break;
        }

        case ID_GAME_ATTACK:
        {
            DEBUG_PRINTF("ID_GAME_ATTACK\n");

            if(!player->CreatureStats()->mDead)
            {
                myPacket->Read(player);

#if defined(DEBUG)
                cout << "Player: " << player->Npc()->mName << " atk state: " << (player->GetAttack()->pressed == 1) <<
                endl;
                if (player->GetAttack()->pressed == 0)
                {
                    cout << "success: " << (player->GetAttack()->success == 1);
                    if (player->GetAttack()->success == 1)
                        cout << " damage: " << player->GetAttack()->damage;
                    cout << endl;
                }
#endif

                myPacket->Send(player, true);
                controller->GetPacket(ID_GAME_UPDATE_BASESTATS)->RequestData(player->GetAttack()->target);
            }
            break;
        }

        case ID_GAME_UPDATE_BASESTATS:
        {
            DEBUG_PRINTF("ID_GAME_UPDATE_BASESTATS\n");
            myPacket->Read(player);
            myPacket->Send(player, true);
            break;
        }

        case ID_GAME_DIE:
        {
            DEBUG_PRINTF("ID_GAME_DIE\n");
            //packetMainStats.Read(player);
            player->CreatureStats()->mDead = true;
            myPacket->Send(player, true);

            Script::Call<Script::CallbackIdentity("OnPlayerDeath")>(player->GetID());

            break;
        }

        case ID_GAME_RESURRECT:
        {
            DEBUG_PRINTF("ID_GAME_RESURRECT\n");
            //packetResurrect.Read(player);
            player->CreatureStats()->mDead = false;
            myPacket->Send(player, true);
            controller->GetPacket(ID_GAME_UPDATE_POS)->RequestData(player->guid);
            controller->GetPacket(ID_GAME_CELL)->RequestData(player->guid);
            //packetMainStats.RequestData(player->guid);

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

            if(result)
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

        default:
            printf("Message with identifier %i has arrived.\n", packet->data[0]);
            break;
    }
}

void Networking::NewPlayer(RakNet::RakNetGUID guid)
{
    Players::NewPlayer(guid);

    controller->GetPacket(ID_USER_MYID)->Send(Players::GetPlayer(guid), false);

    controller->GetPacket(ID_GAME_BASE_INFO)->RequestData(guid);
    //controller->GetPacket(ID_GAME_UPDATE_SKILLS)->RequestData(guid);
    //controller->GetPacket(ID_GAME_UPDATE_BASESTATS)->RequestData(guid);
    controller->GetPacket(ID_GAME_UPDATE_POS)->RequestData(guid);
    controller->GetPacket(ID_GAME_CELL)->RequestData(guid);
    controller->GetPacket(ID_GAME_UPDATE_EQUIPED)->RequestData(guid);

    for(TPlayers::iterator pl = players->begin(); pl != players->end(); pl++)
    {
        if(pl->first == guid) continue;

        controller->GetPacket(ID_GAME_BASE_INFO)->Send(pl->second, guid);
        //controller->GetPacket(ID_GAME_UPDATE_SKILLS)->Send(pl->second, guid);
        //controller->GetPacket(ID_GAME_UPDATE_BASESTATS)->Send(pl->second, guid);
        controller->GetPacket(ID_GAME_UPDATE_POS)->Send(pl->second, guid);
        controller->GetPacket(ID_GAME_CELL)->Send(pl->second, guid);
        controller->GetPacket(ID_GAME_UPDATE_EQUIPED)->Send(pl->second, guid);
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
        if(kbhit() && getch() == '\n')
            break;
        for (packet=peer->Receive(); packet; peer->DeallocatePacket(packet), packet=peer->Receive())
        {
            switch (packet->data[0])
            {
                case ID_REMOTE_DISCONNECTION_NOTIFICATION:
                    printf("Another client has disconnected.\n");
                    break;
                case ID_REMOTE_CONNECTION_LOST:
                    printf("Another client has lost the connection.\n");
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
                    printf("A client lost the connection.\n");
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
