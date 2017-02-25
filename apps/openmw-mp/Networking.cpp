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
#include "MasterClient.hpp"
#include "Cell.hpp"
#include <components/openmw-mp/Version.hpp>

using namespace mwmp;
using namespace std;

Networking *Networking::sThis = 0;

Networking::Networking(RakNet::RakPeerInterface *peer)
{
    sThis = this;
    this->peer = peer;
    players = Players::getPlayers();

    CellController::create();

    playerController = new PlayerPacketController(peer);
    worldController = new WorldPacketController(peer);

    // Set send stream
    playerController->SetStream(0, &bsOut);
    worldController->SetStream(0, &bsOut);

    running = true;
    exitCode = 0;

    Script::Call<Script::CallbackIdentity("OnServerInit")>();

    serverPassword = TES3MP_DEFAULT_PASSW;
}

Networking::~Networking()
{
    Script::Call<Script::CallbackIdentity("OnServerExit")>(false);

    CellController::destroy();

    sThis = 0;
    delete playerController;
    delete worldController;
    LOG_QUIT();
}

void Networking::setServerPassword(std::string passw) noexcept
{
    serverPassword = passw.empty() ? TES3MP_DEFAULT_PASSW : passw;
}

bool Networking::isPassworded() const
{
    return serverPassword != TES3MP_DEFAULT_PASSW;
}

void Networking::processPlayerPacket(RakNet::Packet *packet)
{
    Player *player = Players::getPlayer(packet->guid);

    PlayerPacket *myPacket = playerController->GetPacket(packet->data[0]);

    if (packet->data[0] == ID_HANDSHAKE)
    {

        myPacket->Read(player);

        if (player->isHandshaked())
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Wrong handshake with player %d, name: %s",
                player->getId(),
                player->npc.mName.c_str());
            kickPlayer(player->guid);
            return;
        }

        if (player->passw != serverPassword)
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Wrong server password for player %d, name: %s (pass: %s)",
                player->getId(),
                player->npc.mName.c_str(),
                player->passw.c_str());
            kickPlayer(player->guid);
            return;
        }
        player->setHandshake();
        return;
    }

    if (!player->isHandshaked())
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Have not completed handshake with player %d",
            player->getId());
        //KickPlayer(player->guid);
        return;
    }

    if (packet->data[0] == ID_LOADED)
    {
        player->setLoadState(Player::LOADED);

        static constexpr unsigned int ident = Script::CallbackIdentity("OnPlayerConnect");
        Script::CallBackReturn<ident> result = true;
        Script::Call<ident>(result, Players::getPlayer(packet->guid)->getId());

        if (!result)
        {
            playerController->GetPacket(ID_USER_DISCONNECTED)->Send(Players::getPlayer(packet->guid), false);
            Players::deletePlayer(packet->guid);
            return;
        }
    }
    else if (packet->data[0] == ID_PLAYER_BASEINFO)
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Received ID_PLAYER_BASEINFO about %s",
            player->npc.mName.c_str());

        myPacket->Read(player);
        myPacket->Send(player, true);
    }

    if (player->getLoadState() == Player::NOTLOADED)
        return;
    else if (player->getLoadState() == Player::LOADED)
    {
        player->setLoadState(Player::POSTLOADED);
        newPlayer(packet->guid);
        return;
    }

    switch (packet->data[0])
    {
        case ID_PLAYER_BASEINFO:
        {
        /*LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Received ID_PLAYER_BASEINFO about %s",
        player->npc.mName.c_str());

        myPacket->Read(player);
        myPacket->Send(player, true);*/

        break;
        }
    case ID_PLAYER_POS:
    {
        //DEBUG_PRINTF("ID_PLAYER_POS \n");

        if (!player->creatureStats.mDead)
        {
            myPacket->Read(player);
            //myPacket->Send(player, true); //send to other clients

            player->sendToLoaded(myPacket);

        }

        break;
    }
    case ID_PLAYER_CELL_CHANGE:
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Received ID_PLAYER_CELL_CHANGE from %s",
            player->npc.mName.c_str());

        if (!player->creatureStats.mDead)
        {
            myPacket->Read(player);

            LOG_APPEND(Log::LOG_INFO, "- Moved to %s",
                player->cell.getDescription().c_str());

            player->forEachLoaded([this](Player *pl, Player *other) {

                LOG_APPEND(Log::LOG_INFO, "- Started information exchange with %s",
                    other->npc.mName.c_str());

                playerController->GetPacket(ID_PLAYER_DYNAMICSTATS)->Send(other, pl->guid);
                playerController->GetPacket(ID_PLAYER_ATTRIBUTE)->Send(other, pl->guid);
                playerController->GetPacket(ID_PLAYER_POS)->Send(other, pl->guid);
                playerController->GetPacket(ID_PLAYER_SKILL)->Send(other, pl->guid);
                playerController->GetPacket(ID_PLAYER_EQUIPMENT)->Send(other, pl->guid);
                playerController->GetPacket(ID_PLAYER_DRAWSTATE)->Send(other, pl->guid);

                playerController->GetPacket(ID_PLAYER_DYNAMICSTATS)->Send(pl, other->guid);
                playerController->GetPacket(ID_PLAYER_ATTRIBUTE)->Send(pl, other->guid);
                playerController->GetPacket(ID_PLAYER_SKILL)->Send(pl, other->guid);
                playerController->GetPacket(ID_PLAYER_EQUIPMENT)->Send(pl, other->guid);
                playerController->GetPacket(ID_PLAYER_DRAWSTATE)->Send(pl, other->guid);
                
                LOG_APPEND(Log::LOG_INFO, "- Finished information exchange with %s",
                    other->npc.mName.c_str());
            });

            playerController->GetPacket(ID_PLAYER_POS)->Send(player);
            myPacket->Send(player, true); //send to other clients

            Script::Call<Script::CallbackIdentity("OnPlayerCellChange")>(player->getId());

            LOG_APPEND(Log::LOG_INFO, "- Finished processing ID_PLAYER_CELL_CHANGE",
                player->cell.getDescription().c_str());
        }
        else
        {
            LOG_APPEND(Log::LOG_INFO, "- Ignored because %s is dead",
                player->npc.mName.c_str());
        }

        break;
    }
    case ID_PLAYER_CELL_STATE:
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Received ID_PLAYER_CELL_STATE from %s",
            player->npc.mName.c_str());

        myPacket->Read(player);

        CellController::get()->update(player);

        Script::Call<Script::CallbackIdentity("OnPlayerCellState")>(player->getId());

        break;
    }
    case ID_PLAYER_ATTRIBUTE:
    {

        if (!player->creatureStats.mDead)
        {
            myPacket->Read(player);
            //myPacket->Send(player, true);

            player->sendToLoaded(myPacket);

            Script::Call<Script::CallbackIdentity("OnPlayerAttributesChange")>(player->getId());
        }

        break;
    }
    case ID_PLAYER_SKILL:
    {

        if (!player->creatureStats.mDead)
        {
            myPacket->Read(player);
            //myPacket->Send(player, true);
            player->sendToLoaded(myPacket);

            Script::Call<Script::CallbackIdentity("OnPlayerSkillsChange")>(player->getId());
        }

        break;
    }
    case ID_PLAYER_LEVEL:
    {

        if (!player->creatureStats.mDead)
        {
            myPacket->Read(player);
            //myPacket->Send(player, true);

            Script::Call<Script::CallbackIdentity("OnPlayerLevelChange")>(player->getId());
        }

        break;
    }
    case ID_PLAYER_EQUIPMENT:
    {
        DEBUG_PRINTF("ID_PLAYER_EQUIPMENT\n");

        myPacket->Read(player);
        //myPacket->Send(player, true);

        player->sendToLoaded(myPacket);

        Script::Call<Script::CallbackIdentity("OnPlayerEquipmentChange")>(player->getId());

        break;
    }

    case ID_PLAYER_INVENTORY:
    {
        DEBUG_PRINTF("ID_PLAYER_INVENTORY\n");
        myPacket->Read(player);

        Script::Call<Script::CallbackIdentity("OnPlayerInventoryChange")>(player->getId());

        break;
    }

    case ID_PLAYER_SPELLBOOK:
    {
        DEBUG_PRINTF("ID_PLAYER_SPELLBOOK\n");
        myPacket->Read(player);

        Script::Call<Script::CallbackIdentity("OnPlayerSpellbookChange")>(player->getId());

        break;
    }

    case ID_PLAYER_JOURNAL:
    {
        DEBUG_PRINTF("ID_PLAYER_JOURNAL\n");
        myPacket->Read(player);

        Script::Call<Script::CallbackIdentity("OnPlayerJournalChange")>(player->getId());

        break;
    }

    case ID_PLAYER_ATTACK:
    {
        DEBUG_PRINTF("ID_PLAYER_ATTACK\n");

        if (!player->creatureStats.mDead)
        {
            myPacket->Read(player);

            Player *target = Players::getPlayer(player->attack.target);

            if (target == nullptr)
                target = player;

            LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Player: %s attacked %s state: %d", player->npc.mName.c_str(),
                target->npc.mName.c_str(), player->attack.pressed == 1);
            if (player->attack.pressed == 0)
            {
                LOG_APPEND(Log::LOG_VERBOSE, "success: %d", player->attack.success == 1);
                if (player->attack.success == 1)
                {
                    LOG_APPEND(Log::LOG_VERBOSE, "damage: %d", player->attack.damage == 1);
                    target->setLastAttackerId(player->getId());
                    target->setLastAttackerTime(std::chrono::steady_clock::now());
                }
            }

            //myPacket->Send(player, true);
            player->sendToLoaded(myPacket);
            playerController->GetPacket(ID_PLAYER_DYNAMICSTATS)->RequestData(player->attack.target);
        }
        break;
    }

    case ID_PLAYER_DYNAMICSTATS:
    {
        DEBUG_PRINTF("ID_PLAYER_DYNAMICSTATS\n");
        myPacket->Read(player);
        //myPacket->Send(player, true);

        player->sendToLoaded(myPacket);

        break;
    }

    case ID_GAME_DIE:
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Received ID_GAME_DIE from %s",
            player->npc.mName.c_str());

        Player *killer = Players::getPlayer(player->getLastAttackerId());

        short reason = 0; // unknown;
        double secondsSinceLastAttacker = chrono::duration_cast<chrono::duration<double>>(
            chrono::steady_clock::now() - player->getLastAttackerTime()).count();

        if (!killer)
            killer = player;

        if (secondsSinceLastAttacker < 3.0f)
            reason = 1; // killed
        else
            reason = 2; //suicide

        player->resetLastAttacker();

        player->creatureStats.mDead = true;
        myPacket->Send(player, true);

        Script::Call<Script::CallbackIdentity("OnPlayerDeath")>(player->getId(), reason, killer->getId());

        break;
    }

    case ID_GAME_RESURRECT:
    {
        DEBUG_PRINTF("ID_GAME_RESURRECT\n");
        //packetResurrect.Read(player);
        player->creatureStats.mDead = false;
        myPacket->Send(player, true);
        playerController->GetPacket(ID_PLAYER_POS)->RequestData(player->guid);
        playerController->GetPacket(ID_PLAYER_CELL_CHANGE)->RequestData(player->guid);

        Script::Call<Script::CallbackIdentity("OnPlayerResurrect")>(player->getId());

        break;
    }

    case ID_PLAYER_DRAWSTATE:
    {
        DEBUG_PRINTF("ID_PLAYER_DRAWSTATE\n");
        myPacket->Read(player);
        //myPacket->Send(player, true);

        player->sendToLoaded(myPacket);

        break;
    }

    case ID_CHAT_MESSAGE:
    {
        DEBUG_PRINTF("ID_CHAT_MESSAGE\n");
        myPacket->Read(player);
        Script::CallBackReturn<Script::CallbackIdentity("OnPlayerSendMessage")> result = true;
        Script::Call<Script::CallbackIdentity("OnPlayerSendMessage")>(result, player->getId(), player->chatMessage.c_str());

        if (result)
        {
            player->chatMessage = player->npc.mName + " (" + std::to_string(player->getId()) + "): "
                + player->chatMessage + "\n";
            myPacket->Send(player, false);
            myPacket->Send(player, true);
        }
        break;
    }
    case ID_PLAYER_CHARGEN:
    {
        DEBUG_PRINTF("ID_PLAYER_CHARGEN\n");
        myPacket->Read(player);

        if (player->charGenStage.current == player->charGenStage.end && player->charGenStage.current != 0)
        {
            Script::Call<Script::CallbackIdentity("OnPlayerEndCharGen")>(player->getId());
            cout << "RACE: " << player->npc.mRace << endl;
        }
        break;
    }

    case ID_GUI_MESSAGEBOX:
    {
        DEBUG_PRINTF("ID_GUI_MESSAGEBOX\n");
        myPacket->Read(player);

        Script::Call<Script::CallbackIdentity("OnGUIAction")>(player->getId(), (int)player->guiMessageBox.id,
            player->guiMessageBox.data.c_str());
        break;
    }

    case ID_PLAYER_CHARCLASS:
    {
        DEBUG_PRINTF("ID_PLAYER_CHARCLASS\n");
        myPacket->Read(player);
        break;
    }

    default:
        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Unhandled PlayerPacket with identifier %i has arrived",
            packet->data[0]);
        break;
    }
}

void Networking::processWorldPacket(RakNet::Packet *packet)
{
    Player *player = Players::getPlayer(packet->guid);

    if (!player->isHandshaked() || player->getLoadState() != Player::POSTLOADED)
        return;

    WorldPacket *myPacket = worldController->GetPacket(packet->data[0]);

    // Clear our baseEvent before loading new data in it
    baseEvent.cell.blank();
    baseEvent.objectChanges.objects.clear();
    baseEvent.guid = packet->guid;

    switch (packet->data[0])
    {

    case ID_OBJECT_PLACE:
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_OBJECT_PLACE from %s",
            player->npc.mName.c_str());

        myPacket->Read(&baseEvent);
        myPacket->Send(&baseEvent, true);

        Script::Call<Script::CallbackIdentity("OnObjectPlace")>(
            player->getId(),
            baseEvent.cell.getDescription().c_str());

        break;
    }

    case ID_OBJECT_DELETE:
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_OBJECT_DELETE from %s",
            player->npc.mName.c_str());

        myPacket->Read(&baseEvent);
        myPacket->Send(&baseEvent, true);

        Script::Call<Script::CallbackIdentity("OnObjectDelete")>(
            player->getId(),
            baseEvent.cell.getDescription().c_str());

        break;
    }

    case ID_OBJECT_SCALE:
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_OBJECT_SCALE from %s",
            player->npc.mName.c_str());

        myPacket->Read(&baseEvent);
        myPacket->Send(&baseEvent, true);

        Script::Call<Script::CallbackIdentity("OnObjectScale")>(
            player->getId(),
            baseEvent.cell.getDescription().c_str());

        break;
    }

    case ID_OBJECT_LOCK:
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_OBJECT_LOCK from %s",
            player->npc.mName.c_str());

        myPacket->Read(&baseEvent);
        myPacket->Send(&baseEvent, true);

        Script::Call<Script::CallbackIdentity("OnObjectLock")>(
            player->getId(),
            baseEvent.cell.getDescription().c_str());

        break;
    }

    case ID_OBJECT_UNLOCK:
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_OBJECT_UNLOCK from %s",
            player->npc.mName.c_str());

        myPacket->Read(&baseEvent);
        myPacket->Send(&baseEvent, true);

        Script::Call<Script::CallbackIdentity("OnObjectUnlock")>(
            player->getId(),
            baseEvent.cell.getDescription().c_str());

        break;
    }

    case ID_OBJECT_MOVE:
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_OBJECT_MOVE from %s",
            player->npc.mName.c_str());

        myPacket->Read(&baseEvent);
        myPacket->Send(&baseEvent, true);

        break;
    }

    case ID_OBJECT_ROTATE:
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_OBJECT_ROTATE from %s",
            player->npc.mName.c_str());

        myPacket->Read(&baseEvent);
        myPacket->Send(&baseEvent, true);

        break;
    }

    case ID_OBJECT_ANIM_PLAY:
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_OBJECT_ANIM_PLAY from %s",
            player->npc.mName.c_str());

        myPacket->Read(&baseEvent);
        myPacket->Send(&baseEvent, true);

        break;
    }

    case ID_DOOR_STATE:
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_DOOR_STATE from %s",
            player->npc.mName.c_str());

        myPacket->Read(&baseEvent);
        myPacket->Send(&baseEvent, true);

        Script::Call<Script::CallbackIdentity("OnDoorState")>(
            player->getId(),
            baseEvent.cell.getDescription().c_str());

        break;
    }

    case ID_CONTAINER:
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_CONTAINER from %s",
            player->npc.mName.c_str());

        myPacket->Read(&baseEvent);

        LOG_APPEND(Log::LOG_WARN, "- action: %i", baseEvent.action);

        // Until we have a timestamp-based system, send packets pertaining to more
        // than one container (i.e. replies to server requests for container contents)
        // only to players who have the container's cell loaded
        if (baseEvent.action == BaseEvent::SET && baseEvent.objectChanges.count > 1)
            CellController::get()->getCell(&baseEvent.cell)->sendToLoaded(myPacket, &baseEvent);
        else
            myPacket->Send(&baseEvent, true);

        Script::Call<Script::CallbackIdentity("OnContainer")>(
            player->getId(),
            baseEvent.cell.getDescription().c_str());

        LOG_APPEND(Log::LOG_INFO, "- Finished processing ID_CONTAINER");

        break;
    }

    case ID_SCRIPT_LOCAL_SHORT:
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_SCRIPT_LOCAL_SHORT from %s",
            player->npc.mName.c_str());

        myPacket->Read(&baseEvent);
        myPacket->Send(&baseEvent, true);

        break;
    }

    case ID_SCRIPT_LOCAL_FLOAT:
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_SCRIPT_LOCAL_FLOAT from %s",
            player->npc.mName.c_str());

        myPacket->Read(&baseEvent);
        myPacket->Send(&baseEvent, true);

        break;
    }

    case ID_SCRIPT_MEMBER_SHORT:
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_SCRIPT_MEMBER_SHORT from %s",
            player->npc.mName.c_str());

        myPacket->Read(&baseEvent);
        myPacket->Send(&baseEvent, true);

        break;
    }

    case ID_SCRIPT_GLOBAL_SHORT:
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_SCRIPT_GLOBAL_SHORT from %s",
            player->npc.mName.c_str());

        myPacket->Read(&baseEvent);
        myPacket->Send(&baseEvent, true);

        break;
    }

    case ID_MUSIC_PLAY:
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_MUSIC_PLAY from %s",
            player->npc.mName.c_str());

        myPacket->Read(&baseEvent);
        myPacket->Send(&baseEvent, true);

        break;
    }

    case ID_VIDEO_PLAY:
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_VIDEO_PLAY from %s",
            player->npc.mName.c_str());

        myPacket->Read(&baseEvent);
        myPacket->Send(&baseEvent, true);

        break;
    }

    default:
        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Unhandled WorldPacket with identifier %i has arrived",
            packet->data[0]);
        break;
    }

}

void Networking::update(RakNet::Packet *packet)
{
    Player *player = Players::getPlayer(packet->guid);

    RakNet::BitStream bsIn(&packet->data[1], packet->length, false);

    {
        RakNet::RakNetGUID ignoredGUID;
        bsIn.Read(ignoredGUID);
        (void)ignoredGUID;
    }

    if (player == 0)
    {
        playerController->SetStream(&bsIn, 0);

        playerController->GetPacket(ID_HANDSHAKE)->RequestData(packet->guid);
        Players::newPlayer(packet->guid);
        player = Players::getPlayer(packet->guid);
        playerController->GetPacket(ID_USER_MYID)->Send(Players::getPlayer(packet->guid), false);
        return;
    }
    else if (playerController->ContainsPacket(packet->data[0]))
    {
        playerController->SetStream(&bsIn, 0);
        processPlayerPacket(packet);
    }
    else if (worldController->ContainsPacket(packet->data[0]))
    {
        worldController->SetStream(&bsIn, 0);
        processWorldPacket(packet);
    }
    else
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Unhandled Raknet packet with identifier %i has arrived",
            packet->data[0]);
    }
}

void Networking::newPlayer(RakNet::RakNetGUID guid)
{
    playerController->GetPacket(ID_PLAYER_BASEINFO)->RequestData(guid);
    playerController->GetPacket(ID_PLAYER_DYNAMICSTATS)->RequestData(guid);
    playerController->GetPacket(ID_PLAYER_POS)->RequestData(guid);
    playerController->GetPacket(ID_PLAYER_CELL_CHANGE)->RequestData(guid);
    playerController->GetPacket(ID_PLAYER_EQUIPMENT)->RequestData(guid);

    LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Sending info about other players to %lu",
        guid.g);

    for (TPlayers::iterator pl = players->begin(); pl != players->end(); pl++) //sending other players to new player
    {
        // If we are iterating over the new player, don't send the packets below
        if (pl->first == guid) continue;

        // If an invalid key makes it into the Players map, ignore it
        else if (pl->first == RakNet::UNASSIGNED_RAKNET_GUID) continue;

        // if player not fully connected
        else if (pl->second == nullptr) continue;

        // If we are iterating over a player who has inputted their name, proceed
        else if (pl->second->getLoadState() == Player::POSTLOADED)
        {
            playerController->GetPacket(ID_PLAYER_BASEINFO)->Send(pl->second, guid);
            playerController->GetPacket(ID_PLAYER_DYNAMICSTATS)->Send(pl->second, guid);
            playerController->GetPacket(ID_PLAYER_ATTRIBUTE)->Send(pl->second, guid);
            playerController->GetPacket(ID_PLAYER_SKILL)->Send(pl->second, guid);
            playerController->GetPacket(ID_PLAYER_POS)->Send(pl->second, guid);
            playerController->GetPacket(ID_PLAYER_CELL_CHANGE)->Send(pl->second, guid);
            playerController->GetPacket(ID_PLAYER_EQUIPMENT)->Send(pl->second, guid);
        }
    }

    LOG_APPEND(Log::LOG_WARN, "- Done");

}

void Networking::disconnectPlayer(RakNet::RakNetGUID guid)
{
    Player *player = Players::getPlayer(guid);
    if (!player)
        return;
    Script::Call<Script::CallbackIdentity("OnPlayerDisconnect")>(player->getId());
    playerController->GetPacket(ID_USER_DISCONNECTED)->Send(player, true);
    Players::deletePlayer(guid);
}

PlayerPacketController *Networking::getPlayerController() const
{
    return playerController;
}

WorldPacketController *Networking::getWorldController() const
{
    return worldController;
}

BaseEvent *Networking::getLastEvent()
{
    return &baseEvent;
}

const Networking &Networking::get()
{
    return *sThis;
}


Networking *Networking::getPtr()
{
    return sThis;
}

void Networking::stopServer(int code)
{
    running = false;
    exitCode = code;
}

int Networking::mainLoop()
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
                    LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Client at %s has disconnected",
                        packet->systemAddress.ToString());
                    break;
                case ID_REMOTE_CONNECTION_LOST:
                    LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Client at %s has lost connection",
                        packet->systemAddress.ToString());
                    break;
                case ID_REMOTE_NEW_INCOMING_CONNECTION:
                    LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Client at %s has connected",
                        packet->systemAddress.ToString());
                    break;
                case ID_CONNECTION_REQUEST_ACCEPTED:    // client to server
                {
                    LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Our connection request has been accepted");
                    break;
                }
                case ID_NEW_INCOMING_CONNECTION:
                    LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "A connection is incoming from %s",
                        packet->systemAddress.ToString());
                    break;
                case ID_NO_FREE_INCOMING_CONNECTIONS:
                    LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "The server is full");
                    break;
                case ID_DISCONNECTION_NOTIFICATION:
                    LOG_MESSAGE_SIMPLE(Log::LOG_WARN,  "Client at %s has disconnected",
                        packet->systemAddress.ToString());
                    disconnectPlayer(packet->guid);
                    break;
                case ID_CONNECTION_LOST:
                    LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Client at %s has lost connection",
                        packet->systemAddress.ToString());
                    disconnectPlayer(packet->guid);
                    break;
                case ID_CONNECTED_PING:
                case ID_UNCONNECTED_PING:
                    break;
                case ID_MASTER_QUERY:
                {
                    LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Query request from %s",
                                       packet->systemAddress.ToString());
                    RakNet::BitStream bs;
                    bs.Write((unsigned char) ID_MASTER_QUERY);
                    bs.Write(Players::getPlayers()->size());
                    for (auto player : *Players::getPlayers())
                        bs.Write(RakNet::RakString(player.second->npc.mName.c_str()));
                    bs.Write(0); // plugins
                    peer->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, false);
                    break;
                }
                default:
                    update(packet);
                    break;
            }
        }
        TimerAPI::Tick();
        this_thread::sleep_for(chrono::milliseconds(1));
    }

    TimerAPI::Terminate();
    return exitCode;
}

void Networking::kickPlayer(RakNet::RakNetGUID guid)
{
    peer->CloseConnection(guid, true);
}

unsigned short Networking::numberOfConnections() const
{
    return peer->NumberOfConnections();
}

unsigned int Networking::maxConnections() const
{
    return peer->GetMaximumIncomingConnections();
}

int Networking::getAvgPing(RakNet::AddressOrGUID addr) const
{
    return peer->GetAveragePing(addr);
}

MasterClient *Networking::getMasterClient()
{
    return mclient;
}

void Networking::InitQuery(std::string queryAddr, unsigned short queryPort, std::string serverAddr,
                           unsigned short serverPort)
{
    mclient = new MasterClient(queryAddr, (unsigned short) queryPort, serverAddr, (unsigned short) serverPort);
}
