//
// Created by koncord on 04.01.16.
//

#include <stdexcept>
#include <iostream>
#include <string>
#include <components/esm/cellid.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "../mwworld/cellstore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"

#include "../mwclass/npc.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/combat.hpp"

#include <SDL_messagebox.h>
#include "Networking.hpp"
#include "../mwstate/statemanagerimp.hpp"
#include <components/openmw-mp/Log.hpp>
#include <components/openmw-mp/Version.hpp>
#include "DedicatedPlayer.hpp"
#include "LocalPlayer.hpp"
#include "GUIController.hpp"
#include "WorldController.hpp"
#include "Main.hpp"

using namespace std;
using namespace mwmp;

Networking::Networking(): peer(RakNet::RakPeerInterface::GetInstance()), playerController(peer), worldController(peer)
{

    RakNet::SocketDescriptor sd;
    sd.port=0;
    RakNet::StartupResult b = peer->Startup(1,&sd, 1);
    RakAssert(b==RAKNET_STARTED);

    playerController.SetStream(0, &bsOut);
    worldController.SetStream(0, &bsOut);

    connected = 0;
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

void Networking::sendData(RakNet::BitStream *bs)
{
    peer->Send(bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, serverAddr, false);
}

void Networking::connect(const std::string &ip, unsigned short port)
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
                    LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Connection message with identifier %i has arrived in initialization.", packet->data[0]);
            }
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
    RakNet::RakNetGUID guid;
    RakNet::BitStream bsIn(&packet->data[1], packet->length, false);
    bsIn.Read(guid);

    DedicatedPlayer *pl = 0;
    static RakNet::RakNetGUID myGuid = getLocalPlayer()->guid;
    if (guid != myGuid)
        pl = Players::getPlayer(guid);

    PlayerPacket *myPacket = playerController.GetPacket(packet->data[0]);

    switch (packet->data[0])
    {
    case ID_HANDSHAKE:
    {
        myPacket->Send(getLocalPlayer(), serverAddr);
        break;
    }
    case ID_PLAYER_BASEINFO:
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Received ID_PLAYER_BASEINFO from server");

        if (guid == myGuid)
        {
            LOG_APPEND(Log::LOG_INFO, "- Packet was about my id");

            if (packet->length == myPacket->headerSize())
            {
                LOG_APPEND(Log::LOG_INFO, "- Requesting info");
                myPacket->Send(getLocalPlayer(), serverAddr);
            }
            else
            {
                myPacket->Packet(&bsIn, getLocalPlayer(), false);
                LOG_APPEND(Log::LOG_INFO, "- Updating LocalPlayer");
                getLocalPlayer()->updateChar();
            }
        }
        else
        {
            LOG_APPEND(Log::LOG_INFO, "- Packet was about %s", pl == 0 ? "new player" : pl->npc.mName.c_str());

            if (pl == 0)
            {
                LOG_APPEND(Log::LOG_INFO, "- Exchanging data with new player");
                pl = Players::newPlayer(guid);
            }

            myPacket->Packet(&bsIn, pl, false);
            Players::createPlayer(guid);
        }
        break;
    }
    case ID_PLAYER_POS:
    {
        if (guid == myGuid)
        {
            if (packet->length != myPacket->headerSize())
            {
                LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "ID_PLAYER_POS changed by server");
                myPacket->Packet(&bsIn, getLocalPlayer(), false);
                getLocalPlayer()->setPosition();
            }
            else
                getLocalPlayer()->updatePosition(true);
        }
        else if (pl != 0)
        {
            myPacket->Packet(&bsIn, pl, false);
            pl->updateMarker();
        }
        break;
    }
    case ID_USER_MYID:
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Received ID_USER_MYID from server");
        myGuid = guid;
        getLocalPlayer()->guid = guid;
        break;
    }
    case ID_USER_DISCONNECTED:
    {
        if (guid == myGuid)
            MWBase::Environment::get().getStateManager()->requestQuit();
        else if (pl != 0)
            Players::disconnectPlayer(guid);

    }
    case ID_PLAYER_EQUIPMENT:
    {
        if (guid == myGuid)
        {
            if (packet->length == myPacket->headerSize())
            {
                getLocalPlayer()->updateEquipment(true);
            }
            else
            {
                myPacket->Packet(&bsIn, getLocalPlayer(), false);
                getLocalPlayer()->setEquipment();
            }
        }
        else if (pl != 0)
        {
            myPacket->Packet(&bsIn, pl, false);
            pl->updateEquipment();
        }
        break;
    }
    case ID_PLAYER_INVENTORY:
    {
        if (guid == myGuid)
        {
            if (packet->length == myPacket->headerSize())
            {
                getLocalPlayer()->updateInventory(true);
            }
            else
            {
                myPacket->Packet(&bsIn, getLocalPlayer(), false);
                int inventoryAction = getLocalPlayer()->inventoryChanges.action;

                if (inventoryAction == InventoryChanges::ADD)
                {
                    getLocalPlayer()->addItems();
                }
                else if (inventoryAction == InventoryChanges::REMOVE)
                {
                    getLocalPlayer()->removeItems();
                }
                else // InventoryChanges::SET
                {
                    getLocalPlayer()->setInventory();
                }
            }
        }
        break;
    }
    case ID_PLAYER_SPELLBOOK:
    {
        if (guid == myGuid)
        {
            if (packet->length == myPacket->headerSize())
            {
                getLocalPlayer()->sendSpellbook();
            }
            else
            {
                myPacket->Packet(&bsIn, getLocalPlayer(), false);
                int spellbookAction = getLocalPlayer()->spellbookChanges.action;

                if (spellbookAction == SpellbookChanges::ADD)
                {
                    getLocalPlayer()->addSpells();
                }
                else if (spellbookAction == SpellbookChanges::REMOVE)
                {
                    getLocalPlayer()->removeSpells();
                }
                else // SpellbookChanges::SET
                {
                    getLocalPlayer()->setSpellbook();
                }
            }
        }
        break;
    }
    case ID_PLAYER_JOURNAL:
    {
        if (guid == myGuid)
        {
            if (packet->length == myPacket->headerSize())
            {
                // Entire journal cannot currently be requested from players
            }
            else
            {
                myPacket->Packet(&bsIn, getLocalPlayer(), false);
                getLocalPlayer()->addJournalItems();
            }
        }
        break;
    }
    case ID_PLAYER_ATTACK:
    {
        if (pl != 0)
        {
            myPacket->Packet(&bsIn, pl, false);

            //cout << "Player: " << pl->Npc()->mName << " pressed: " << (pl->getAttack()->pressed == 1) << endl;
            if (pl->attack.pressed == 0)
            {
                LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Attack success: %s",
                    pl->attack.success ? "true" : "false");

                if (pl->attack.success == 1)
                {
                    LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Damage: %f",
                        pl->attack.damage);
                }
            }

            MWMechanics::CreatureStats &stats = pl->getPtr().getClass().getNpcStats(pl->getPtr());
            stats.getSpells().setSelectedSpell(pl->attack.refid);

            MWWorld::Ptr victim;
            if (pl->attack.target == getLocalPlayer()->guid)
                victim = MWBase::Environment::get().getWorld()->getPlayerPtr();
            else if (Players::getPlayer(pl->attack.target) != 0)
                victim = Players::getPlayer(pl->attack.target)->getPtr();

            MWWorld::Ptr attacker;
            attacker = pl->getPtr();

            // Get the weapon used (if hand-to-hand, weapon = inv.end())
            if (pl->drawState == 1)
            {
                MWWorld::InventoryStore &inv = attacker.getClass().getInventoryStore(attacker);
                MWWorld::ContainerStoreIterator weaponslot = inv.getSlot(
                    MWWorld::InventoryStore::Slot_CarriedRight);
                MWWorld::Ptr weapon = ((weaponslot != inv.end()) ? *weaponslot : MWWorld::Ptr());
                if (!weapon.isEmpty() && weapon.getTypeName() != typeid(ESM::Weapon).name())
                    weapon = MWWorld::Ptr();

                if (victim.mRef != 0)
                {
                    bool healthdmg;
                    if (!weapon.isEmpty())
                        healthdmg = true;
                    else
                    {
                        MWMechanics::CreatureStats &otherstats = victim.getClass().getCreatureStats(victim);
                        healthdmg = otherstats.isParalyzed() || otherstats.getKnockedDown();
                    }

                    if (!weapon.isEmpty())
                        MWMechanics::blockMeleeAttack(attacker, victim, weapon, pl->attack.damage, 1);
                    pl->getPtr().getClass().onHit(victim, pl->attack.damage, healthdmg, weapon, attacker, osg::Vec3f(),
                        pl->attack.success);
                }
            }
            else
            {
                LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "SpellId: %s",
                    pl->attack.refid.c_str());
                LOG_APPEND(Log::LOG_VERBOSE, " - success: %d", pl->attack.success);
            }
        }
        break;
    }
    case ID_PLAYER_DYNAMICSTATS:
    {
        if (guid == myGuid)
        {
            if (packet->length == myPacket->headerSize())
            {
                getLocalPlayer()->updateDynamicStats(true);
            }
            else
            {
                myPacket->Packet(&bsIn, getLocalPlayer(), false);
                getLocalPlayer()->setDynamicStats();
            }
        }
        else if (pl != 0)
        {
            myPacket->Packet(&bsIn, pl, false);

            MWWorld::Ptr ptrPlayer = pl->getPtr();
            MWMechanics::CreatureStats *ptrCreatureStats = &ptrPlayer.getClass().getCreatureStats(ptrPlayer);
            MWMechanics::DynamicStat<float> value;

            for (int i = 0; i < 3; ++i)
            {
                value.readState(pl->creatureStats.mDynamic[i]);
                ptrCreatureStats->setDynamic(i, value);
            }
        }
        break;
    }
    case ID_GAME_DIE:
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Received ID_GAME_DIE from server");
        if (guid == myGuid)
        {
            MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
            MWMechanics::DynamicStat<float> health = player.getClass().getCreatureStats(player).getHealth();
            health.setCurrent(0);
            player.getClass().getCreatureStats(player).setHealth(health);
            myPacket->Send(getLocalPlayer(), serverAddr);
        }
        else if (pl != 0)
        {
            LOG_APPEND(Log::LOG_INFO, "- Packet was about %s", pl->npc.mName.c_str());
            MWMechanics::DynamicStat<float> health;
            pl->creatureStats.mDead = true;
            health.readState(pl->creatureStats.mDynamic[0]);
            health.setCurrent(0);
            health.writeState(pl->creatureStats.mDynamic[0]);
            pl->getPtr().getClass().getCreatureStats(pl->getPtr()).setHealth(health);
        }
        break;
    }
    case ID_GAME_RESURRECT:
    {
        if (guid == myGuid)
        {
            MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
            player.getClass().getCreatureStats(player).resurrect();
            ESM::Position resurrectPos;
            MWBase::Environment::get().getWorld()->findInteriorPosition("Pelagiad, Fort Pelagiad", resurrectPos);
            MWBase::Environment::get().getWorld()->changeToInteriorCell("Pelagiad, Fort Pelagiad", resurrectPos, true);

            // If this player had a weapon or spell readied when dying, they will
            // still have it readied but be unable to use it unless we clear it here
            player.getClass().getNpcStats(player).setDrawState(MWMechanics::DrawState_Nothing);

            getLocalPlayer()->position = resurrectPos;
            getLocalPlayer()->cell = *player.getCell()->getCell();
            myPacket->Send(getLocalPlayer(), serverAddr);

            getLocalPlayer()->updateDynamicStats(true);
            playerController.GetPacket(ID_PLAYER_DYNAMICSTATS)->Send(getLocalPlayer(), serverAddr);
        }
        else if (pl != 0)
        {
            pl->creatureStats.mDead = false;
            if (pl->creatureStats.mDynamic[0].mMod < 1)
                pl->creatureStats.mDynamic[0].mMod = 1;
            pl->creatureStats.mDynamic[0].mCurrent = pl->creatureStats.mDynamic[0].mMod;

            pl->getPtr().getClass().getCreatureStats(pl->getPtr()).resurrect();

            MWMechanics::DynamicStat<float> health;
            health.readState(pl->creatureStats.mDynamic[0]);
            pl->getPtr().getClass().getCreatureStats(pl->getPtr()).setHealth(health);
        }
        break;
    }
    case ID_PLAYER_CELL_CHANGE:
    {
        if (guid == myGuid)
        {
            if (packet->length == myPacket->headerSize())
                getLocalPlayer()->updateCell(true);
            else
            {
                myPacket->Packet(&bsIn, getLocalPlayer(), false);
                getLocalPlayer()->setCell();
            }
        }
        else if (pl != 0)
        {
            myPacket->Packet(&bsIn, pl, false);
            pl->updateCell();
        }
        break;
    }
    case ID_PLAYER_CELL_STATE:
    {
        if (guid == myGuid)
        {
            if (packet->length == myPacket->headerSize())
                getLocalPlayer()->sendCellStates();
        }

        break;
    }
    case ID_PLAYER_DRAWSTATE:
    {
        if (guid == myGuid)
            getLocalPlayer()->updateDrawStateAndFlags(true);
        else if (pl != 0)
        {
            myPacket->Packet(&bsIn, pl, false);
            pl->updateDrawState();
        }
        break;
    }
    case ID_CHAT_MESSAGE:
    {
        std::string message;
        if (guid == myGuid)
        {
            myPacket->Packet(&bsIn, getLocalPlayer(), false);
            message = getLocalPlayer()->chatMessage;
        }
        else if (pl != 0)
        {
            myPacket->Packet(&bsIn, pl, false);
            message = pl->chatMessage;
        }
        Main::get().getGUIController()->printChatMessage(message);

        break;
    }
    case ID_PLAYER_CHARGEN:
    {
        if (guid == myGuid)
        {
            myPacket->Packet(&bsIn, getLocalPlayer(), false);
        }
        break;
    }
    case ID_PLAYER_ATTRIBUTE:
    {
        if (guid == myGuid)
        {
            if (packet->length == myPacket->headerSize())
            {
                getLocalPlayer()->updateAttributes(true);
            }
            else
            {
                myPacket->Packet(&bsIn, getLocalPlayer(), false);
                getLocalPlayer()->setAttributes();
            }
        }
        else if (pl != 0)
        {
            myPacket->Packet(&bsIn, pl, false);

            MWWorld::Ptr ptrPlayer = pl->getPtr();
            MWMechanics::CreatureStats *ptrCreatureStats = &ptrPlayer.getClass().getCreatureStats(ptrPlayer);
            MWMechanics::AttributeValue attributeValue;

            for (int i = 0; i < 8; ++i)
            {
                attributeValue.readState(pl->creatureStats.mAttributes[i]);
                ptrCreatureStats->setAttribute(i, attributeValue);
            }
        }
        break;
    }
    case ID_PLAYER_SKILL:
    {
        if (guid == myGuid)
        {
            if (packet->length == myPacket->headerSize())
            {
                getLocalPlayer()->updateSkills(true);
            }
            else
            {
                myPacket->Packet(&bsIn, getLocalPlayer(), false);
                getLocalPlayer()->setSkills();
            }
        }
        else if (pl != 0)
        {
            myPacket->Packet(&bsIn, pl, false);

            MWWorld::Ptr ptrPlayer = pl->getPtr();
            MWMechanics::NpcStats *ptrNpcStats = &ptrPlayer.getClass().getNpcStats(ptrPlayer);
            MWMechanics::SkillValue skillValue;

            for (int i = 0; i < 27; ++i)
            {
                skillValue.readState(pl->npcStats.mSkills[i]);
                ptrNpcStats->setSkill(i, skillValue);
            }
        }
        break;
    }
    case ID_PLAYER_LEVEL:
    {
        if (guid == myGuid)
        {
            if (packet->length == myPacket->headerSize())
            {
                getLocalPlayer()->updateLevel(true);
            }
            else
            {
                myPacket->Packet(&bsIn, getLocalPlayer(), false);
                getLocalPlayer()->setLevel();
            }
        }
        else if (pl != 0)
        {
            myPacket->Packet(&bsIn, pl, false);

            MWWorld::Ptr ptrPlayer = pl->getPtr();
            MWMechanics::CreatureStats *ptrCreatureStats = &ptrPlayer.getClass().getCreatureStats(ptrPlayer);

            ptrCreatureStats->setLevel(pl->creatureStats.mLevel);
        }
        break;
    }
    case ID_GUI_MESSAGEBOX:
    {
        if (guid == myGuid)
        {
            myPacket->Packet(&bsIn, getLocalPlayer(), false);

            LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "ID_GUI_MESSAGEBOX, Type %d, MSG %s",
                getLocalPlayer()->guiMessageBox.type,
                getLocalPlayer()->guiMessageBox.label.c_str());

            int messageBoxType = getLocalPlayer()->guiMessageBox.type;

            if (messageBoxType == BasePlayer::GUIMessageBox::MessageBox)
                Main::get().getGUIController()->showMessageBox(getLocalPlayer()->guiMessageBox);
            else if (messageBoxType == BasePlayer::GUIMessageBox::CustomMessageBox)
                Main::get().getGUIController()->showCustomMessageBox(getLocalPlayer()->guiMessageBox);
            else if (messageBoxType == BasePlayer::GUIMessageBox::InputDialog)
                Main::get().getGUIController()->showInputBox(getLocalPlayer()->guiMessageBox);
            else if (messageBoxType == BasePlayer::GUIMessageBox::ListBox)
                Main::get().getGUIController()->showDialogList(getLocalPlayer()->guiMessageBox);
        }
        break;
    }
    case ID_PLAYER_CHARCLASS:
    {
        if (guid == myGuid)
        {
            if (packet->length == myPacket->headerSize())
                getLocalPlayer()->sendClass();
            else
            {
                myPacket->Packet(&bsIn, getLocalPlayer(), false);
                getLocalPlayer()->setClass();
            }
        }
        break;
    }
    case ID_GAME_TIME:
    {
        if (guid == myGuid)
        {
            myPacket->Packet(&bsIn, getLocalPlayer(), false);
            MWBase::World *world = MWBase::Environment::get().getWorld();
            if (getLocalPlayer()->hour != -1)
                world->setHour(getLocalPlayer()->hour);
            else if (getLocalPlayer()->day != -1)
                world->setDay(getLocalPlayer()->day);
            else if (getLocalPlayer()->month != -1)
                world->setMonth(getLocalPlayer()->month);
        }
        break;
    }
    case ID_GAME_CONSOLE:
    {
        myPacket->Packet(&bsIn, getLocalPlayer(), false);
        break;
    }
    default:
        LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Unhandled PlayerPacket with identifier %i has arrived",
            packet->data[0]);
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
        pl = Players::getPlayer(guid);

    WorldPacket *myPacket = worldController.GetPacket(packet->data[0]);

    myPacket->Packet(&bsIn, &worldEvent, false);

    switch (packet->data[0])
    {
    case ID_CONTAINER:
    {
        MWWorld::CellStore *ptrCellStore = Main::get().getWorldController()->getCell(worldEvent.cell);

        if (!ptrCellStore) return;

        LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Received ID_CONTAINER about %s",
            worldEvent.cell.getDescription().c_str());
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
        MWWorld::CellStore *ptrCellStore = Main::get().getWorldController()->getCell(worldEvent.cell);

        if (!ptrCellStore) return;

        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_OBJECT_PLACE about %s",
            worldEvent.cell.getDescription().c_str());
        worldEvent.placeObjects(ptrCellStore);

        break;
    }
    case ID_OBJECT_DELETE:
    {
        MWWorld::CellStore *ptrCellStore = Main::get().getWorldController()->getCell(worldEvent.cell);

        if (!ptrCellStore) return;

        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_OBJECT_DELETE about %s",
            worldEvent.cell.getDescription().c_str());
        worldEvent.deleteObjects(ptrCellStore);

        break;
    }
    case ID_OBJECT_LOCK:
    {
        MWWorld::CellStore *ptrCellStore = Main::get().getWorldController()->getCell(worldEvent.cell);

        if (!ptrCellStore) return;

        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_OBJECT_LOCK about %s",
            worldEvent.cell.getDescription().c_str());
        worldEvent.lockObjects(ptrCellStore);

        break;
    }
    case ID_OBJECT_UNLOCK:
    {
        MWWorld::CellStore *ptrCellStore = Main::get().getWorldController()->getCell(worldEvent.cell);

        if (!ptrCellStore) return;

        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_OBJECT_UNLOCK about %s",
            worldEvent.cell.getDescription().c_str());
        worldEvent.unlockObjects(ptrCellStore);

        break;
    }
    case ID_OBJECT_SCALE:
    {
        MWWorld::CellStore *ptrCellStore = Main::get().getWorldController()->getCell(worldEvent.cell);

        if (!ptrCellStore) return;

        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_OBJECT_SCALE about %s",
            worldEvent.cell.getDescription().c_str());
        worldEvent.scaleObjects(ptrCellStore);

        break;
    }
    case ID_OBJECT_MOVE:
    {
        MWWorld::CellStore *ptrCellStore = Main::get().getWorldController()->getCell(worldEvent.cell);

        if (!ptrCellStore) return;

        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_OBJECT_MOVE about %s",
            worldEvent.cell.getDescription().c_str());
        worldEvent.moveObjects(ptrCellStore);

        break;
    }
    case ID_OBJECT_ROTATE:
    {
        MWWorld::CellStore *ptrCellStore = Main::get().getWorldController()->getCell(worldEvent.cell);

        if (!ptrCellStore) return;

        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_OBJECT_ROTATE about %s",
            worldEvent.cell.getDescription().c_str());
        worldEvent.rotateObjects(ptrCellStore);

        break;
    }
    case ID_OBJECT_ANIM_PLAY:
    {
        MWWorld::CellStore *ptrCellStore = Main::get().getWorldController()->getCell(worldEvent.cell);

        if (!ptrCellStore) return;

        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_OBJECT_ANIM_PLAY about %s",
            worldEvent.cell.getDescription().c_str());
        worldEvent.animateObjects(ptrCellStore);

        break;
    }
    case ID_DOOR_STATE:
    {
        MWWorld::CellStore *ptrCellStore = Main::get().getWorldController()->getCell(worldEvent.cell);

        if (!ptrCellStore) return;

        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_DOOR_STATE about %s",
            worldEvent.cell.getDescription().c_str());
        worldEvent.activateDoors(ptrCellStore);

        break;
    }
    case ID_SCRIPT_LOCAL_SHORT:
    {
        MWWorld::CellStore *ptrCellStore = Main::get().getWorldController()->getCell(worldEvent.cell);

        if (!ptrCellStore) return;

        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_SCRIPT_LOCAL_SHORT about %s",
            worldEvent.cell.getDescription().c_str());
        worldEvent.setLocalShorts(ptrCellStore);

        break;
    }
    case ID_SCRIPT_LOCAL_FLOAT:
    {
        MWWorld::CellStore *ptrCellStore = Main::get().getWorldController()->getCell(worldEvent.cell);

        if (!ptrCellStore) return;

        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_SCRIPT_LOCAL_FLOAT about %s",
            worldEvent.cell.getDescription().c_str());
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
        LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Unhandled WorldPacket with identifier %i has arrived",
            packet->data[0]);
    }
}

void Networking::receiveMessage(RakNet::Packet *packet)
{
    if (packet->length < 2)
        return;

    if (playerController.ContainsPacket(packet->data[0]))
    {
        processPlayerPacket(packet);
    }
    else if (worldController.ContainsPacket(packet->data[0]))
    {
        processWorldPacket(packet);
    }
}

PlayerPacket *Networking::getPlayerPacket(RakNet::MessageID id)
{
    return playerController.GetPacket(id);
}

WorldPacket *Networking::getWorldPacket(RakNet::MessageID id)
{
    return worldController.GetPacket(id);
}

LocalPlayer *Networking::getLocalPlayer()
{
    return mwmp::Main::get().getLocalPlayer();
}

WorldEvent *Networking::resetWorldEvent()
{
    worldEvent.cell.blank();
    worldEvent.objectChanges.objects.clear();
    worldEvent.guid = getLocalPlayer()->guid;
    return &worldEvent;
}

bool Networking::isDedicatedPlayer(const MWWorld::Ptr &ptr)
{
    if (ptr.mRef == 0)
        return 0;
    DedicatedPlayer *pl = Players::getPlayer(ptr);

    return pl != 0;
}

bool Networking::attack(const MWWorld::Ptr &ptr)
{
    DedicatedPlayer *pl = Players::getPlayer(ptr);

    if (pl == 0)
        return false;

    return pl->attack.pressed;
}

bool Networking::isConnected()
{
    return connected;
}
