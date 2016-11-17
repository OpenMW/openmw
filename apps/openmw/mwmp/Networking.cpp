//
// Created by koncord on 04.01.16.
//

#include <stdexcept>
#include <iostream>
#include <string>
#include <components/esm/cellid.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"

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
#include <components/openmw-mp/Base/WorldEvent.hpp>
#include "DedicatedPlayer.hpp"
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
        MWBase::Environment::get().getStateManager()->requestQuit();
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
        (*getLocalPlayer()->getPassw()) = "SuperPassword";
        myPacket->Send(getLocalPlayer(), serverAddr);
        break;
    }
    case ID_GAME_BASE_INFO:
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Received ID_GAME_BASE_INFO from server");

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
            LOG_APPEND(Log::LOG_INFO, "- Packet was about %s", pl == 0 ? "new player" : pl->Npc()->mName.c_str());

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
    case ID_GAME_POS:
    {
        if (guid == myGuid)
        {
            if (packet->length != myPacket->headerSize())
            {
                LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "ID_GAME_POS changed by server");
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
    case ID_GAME_EQUIPMENT:
    {
        if (guid == myGuid)
        {
            if (packet->length == myPacket->headerSize())
            {
                getLocalPlayer()->updateEquipped(true);
            }
            else
            {
                myPacket->Packet(&bsIn, getLocalPlayer(), false);
                getLocalPlayer()->setInventory();
            }
        }
        else if (pl != 0)
        {
            myPacket->Packet(&bsIn, pl, false);
            pl->updateInventory();
        }
        break;
    }
    case ID_GAME_INVENTORY:
    {
        if (guid == myGuid)
        {
            if (packet->length == myPacket->headerSize())
            {
                printf("ID_GAME_INVENTORY update only\n");
                getLocalPlayer()->updateInventory(true);
                getPlayerPacket(ID_GAME_INVENTORY)->Send(getLocalPlayer());
            }
            else
            {
                myPacket->Packet(&bsIn, getLocalPlayer(), false);
                MWWorld::Ptr ptr = MWBase::Environment::get().getWorld()->getPlayerPtr();
                MWWorld::ContainerStore &conStore = ptr.getClass().getContainerStore(ptr);
                if (getLocalPlayer()->inventory.action == Inventory::ADDITEM)
                {
                    for (unsigned int i = 0; i < getLocalPlayer()->inventory.count; i++)
                    {
                        mwmp::Item item = getLocalPlayer()->inventory.items[i];
                        MWWorld::Ptr itemPtr = *conStore.add(item.refid, item.count, ptr);
                        if (item.health != -1)
                            itemPtr.getCellRef().setCharge(item.health);
                    }
                }
                else if (getLocalPlayer()->inventory.action == Inventory::REMOVEITEM)
                {
                    for (unsigned int i = 0; i < getLocalPlayer()->inventory.count; i++)
                    {
                        mwmp::Item item = getLocalPlayer()->inventory.items[i];
                        conStore.remove(item.refid, item.count, ptr);
                    }
                }
                else // update
                {
                    conStore.clear();
                    for (unsigned int i = 0; i < getLocalPlayer()->inventory.count; i++)
                    {
                        mwmp::Item item = getLocalPlayer()->inventory.items[i];
                        MWWorld::Ptr itemPtr = *conStore.add(item.refid, item.count, ptr);
                        if (item.health != -1)
                            itemPtr.getCellRef().setCharge(item.health);
                        printf("%s %d %d\n", item.refid.c_str(), item.count, item.health);
                    }
                    getLocalPlayer()->setInventory(); // restore equipped items
                }
            }
        }
        break;
    }
    case ID_GAME_ATTACK:
    {
        if (pl != 0)
        {
            myPacket->Packet(&bsIn, pl, false);

            //cout << "Player: " << pl->Npc()->mName << " pressed: " << (pl->getAttack()->pressed == 1) << endl;
            if (pl->getAttack()->pressed == 0)
            {
                LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Attack success: %s",
                    pl->getAttack()->success ? "true" : "false");

                if (pl->getAttack()->success == 1)
                {
                    LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Damage: %f",
                        pl->getAttack()->damage);
                }
            }

            MWMechanics::CreatureStats &stats = pl->getPtr().getClass().getNpcStats(pl->getPtr());
            stats.getSpells().setSelectedSpell(pl->getAttack()->refid);

            MWWorld::Ptr victim;
            if (pl->getAttack()->target == getLocalPlayer()->guid)
                victim = MWBase::Environment::get().getWorld()->getPlayerPtr();
            else if (Players::getPlayer(pl->getAttack()->target) != 0)
                victim = Players::getPlayer(pl->getAttack()->target)->getPtr();

            MWWorld::Ptr attacker;
            attacker = pl->getPtr();

            // Get the weapon used (if hand-to-hand, weapon = inv.end())
            if (*pl->DrawState() == 1)
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
                        MWMechanics::blockMeleeAttack(attacker, victim, weapon, pl->getAttack()->damage, 1);
                    pl->getPtr().getClass().onHit(victim, pl->getAttack()->damage, healthdmg, weapon, attacker, osg::Vec3f(),
                        pl->getAttack()->success);
                }
            }
            else
            {
                LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "SpellId: %s",
                    pl->getAttack()->refid.c_str());
                LOG_APPEND(Log::LOG_VERBOSE, " - success: %d", pl->getAttack()->success);
            }
        }
        break;
    }
    case ID_GAME_DYNAMICSTATS:
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
                value.readState(pl->CreatureStats()->mDynamic[i]);
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
            LOG_APPEND(Log::LOG_INFO, "- Packet was about %s", pl->Npc()->mName.c_str());
            MWMechanics::DynamicStat<float> health;
            pl->CreatureStats()->mDead = true;
            health.readState(pl->CreatureStats()->mDynamic[0]);
            health.setCurrent(0);
            health.writeState(pl->CreatureStats()->mDynamic[0]);
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
            ESM::Position pos;
            MWBase::Environment::get().getWorld()->findInteriorPosition("Pelagiad, Fort Pelagiad", pos);
            MWBase::Environment::get().getWorld()->changeToInteriorCell("Pelagiad, Fort Pelagiad", pos, true);
            (*getLocalPlayer()->Position()) = pos;
            (*getLocalPlayer()->getCell()) = *player.getCell()->getCell();
            myPacket->Send(getLocalPlayer(), serverAddr);

            getLocalPlayer()->updateDynamicStats(true);
            playerController.GetPacket(ID_GAME_DYNAMICSTATS)->Send(getLocalPlayer(), serverAddr);
        }
        else if (pl != 0)
        {
            pl->CreatureStats()->mDead = false;
            if (pl->CreatureStats()->mDynamic[0].mMod < 1)
                pl->CreatureStats()->mDynamic[0].mMod = 1;
            pl->CreatureStats()->mDynamic[0].mCurrent = pl->CreatureStats()->mDynamic[0].mMod;

            pl->getPtr().getClass().getCreatureStats(pl->getPtr()).resurrect();

            MWMechanics::DynamicStat<float> health;
            health.readState(pl->CreatureStats()->mDynamic[0]);
            pl->getPtr().getClass().getCreatureStats(pl->getPtr()).setHealth(health);
        }
        break;
    }
    case ID_GAME_CELL:
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
    case ID_GAME_DRAWSTATE:
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
            message = *getLocalPlayer()->ChatMessage();
        }
        else if (pl != 0)
        {
            myPacket->Packet(&bsIn, pl, false);
            message = *pl->ChatMessage();
        }
        Main::get().getGUIController()->printChatMessage(message);

        break;
    }
    case ID_GAME_CHARGEN:
    {
        if (guid == myGuid)
        {
            myPacket->Packet(&bsIn, getLocalPlayer(), false);
        }
        break;
    }
    case ID_GAME_ATTRIBUTE:
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
                attributeValue.readState(pl->CreatureStats()->mAttributes[i]);
                ptrCreatureStats->setAttribute(i, attributeValue);
            }
        }
        break;
    }
    case ID_GAME_SKILL:
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
                skillValue.readState(pl->NpcStats()->mSkills[i]);
                ptrNpcStats->setSkill(i, skillValue);
            }
        }
        break;
    }
    case ID_GAME_LEVEL:
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

            ptrCreatureStats->setLevel(pl->CreatureStats()->mLevel);
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

            if (getLocalPlayer()->guiMessageBox.type == BasePlayer::GUIMessageBox::MessageBox)
                Main::get().getGUIController()->showMessageBox(getLocalPlayer()->guiMessageBox);
            else if (getLocalPlayer()->guiMessageBox.type == BasePlayer::GUIMessageBox::CustomMessageBox)
                Main::get().getGUIController()->showCustomMessageBox(getLocalPlayer()->guiMessageBox);
            else if (getLocalPlayer()->guiMessageBox.type == BasePlayer::GUIMessageBox::InputDialog)
                Main::get().getGUIController()->showInputBox(getLocalPlayer()->guiMessageBox);
            else if (getLocalPlayer()->guiMessageBox.type == BasePlayer::GUIMessageBox::ListBox)
                Main::get().getGUIController()->showDialogList(getLocalPlayer()->guiMessageBox);
        }
        break;
    }
    case ID_GAME_CHARCLASS:
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
    WorldEvent *event = new WorldEvent(guid);
    myPacket->Packet(&bsIn, event, false);

    switch (packet->data[0])
    {
    case ID_OBJECT_PLACE:
    {
        MWWorld::CellStore *ptrCellStore = Main::get().getWorldController()->getCell(event->cell);

        if (!ptrCellStore) return;

        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_OBJECT_PLACE");
        LOG_APPEND(Log::LOG_WARN, "- cellRef: %s, %i\n- cell: %s\n- count: %i",
            event->cellRef.mRefID.c_str(),
            event->cellRef.mRefNum.mIndex,
            event->cell.getDescription().c_str(),
            event->count);

        MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(), event->cellRef.mRefID, 1);
        MWWorld::Ptr newPtr = ref.getPtr();

        if (event->count > 1)
            newPtr.getRefData().setCount(event->count);

        newPtr.getCellRef().setGoldValue(event->cellRef.mGoldValue);

        newPtr = MWBase::Environment::get().getWorld()->placeObject(newPtr, ptrCellStore, event->pos);

        // Change RefNum here because the line above unsets it
        newPtr.getCellRef().setRefNumIndex(event->cellRef.mRefNum.mIndex);

        // If this RefNum is higher than the last we've recorded for this CellStore,
        // start using it as our new last one
        if (ptrCellStore->getLastRefNumIndex() < event->cellRef.mRefNum.mIndex)
            ptrCellStore->setLastRefNumIndex(event->cellRef.mRefNum.mIndex);

        break;
    }
    case ID_OBJECT_DELETE:
    {
        MWWorld::CellStore *ptrCellStore = Main::get().getWorldController()->getCell(event->cell);

        if (!ptrCellStore) return;

        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_OBJECT_DELETE");
        LOG_APPEND(Log::LOG_WARN, "- cellRef: %s, %i\n- cell: %s",
            event->cellRef.mRefID.c_str(),
            event->cellRef.mRefNum.mIndex,
            event->cell.getDescription().c_str());

        MWWorld::Ptr ptrFound = ptrCellStore->searchExact(event->cellRef.mRefID, event->cellRef.mRefNum.mIndex);

        if (ptrFound)
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Found %s, %i",
                ptrFound.getCellRef().getRefId().c_str(),
                ptrFound.getCellRef().getRefNum());

            MWBase::Environment::get().getWorld()->deleteObject(ptrFound);
        }

        break;
    }
    case ID_OBJECT_LOCK:
    {
        MWWorld::CellStore *ptrCellStore = Main::get().getWorldController()->getCell(event->cell);

        if (!ptrCellStore) return;

        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_OBJECT_LOCK");
        LOG_APPEND(Log::LOG_WARN, "- cellRef: %s, %i\n- cell: %s",
            event->cellRef.mRefID.c_str(),
            event->cellRef.mRefNum.mIndex,
            event->cell.getDescription().c_str());

        MWWorld::Ptr ptrFound = ptrCellStore->searchExact(event->cellRef.mRefID, event->cellRef.mRefNum.mIndex);

        if (ptrFound)
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Found %s, %i",
                ptrFound.getCellRef().getRefId().c_str(),
                ptrFound.getCellRef().getRefNum());

            ptrFound.getClass().lock(ptrFound, event->lockLevel);
        }

        break;
    }
    case ID_OBJECT_UNLOCK:
    {
        MWWorld::CellStore *ptrCellStore = Main::get().getWorldController()->getCell(event->cell);

        if (!ptrCellStore) return;

        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_OBJECT_UNLOCK");
        LOG_APPEND(Log::LOG_WARN, "- cellRef: %s, %i\n- cell: %s",
            event->cellRef.mRefID.c_str(),
            event->cellRef.mRefNum.mIndex,
            event->cell.getDescription().c_str());

        MWWorld::Ptr ptrFound = ptrCellStore->searchExact(event->cellRef.mRefID, event->cellRef.mRefNum.mIndex);

        if (ptrFound)
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Found %s, %i",
                ptrFound.getCellRef().getRefId().c_str(),
                ptrFound.getCellRef().getRefNum());

            ptrFound.getClass().unlock(ptrFound);
        }

        break;
    }
    case ID_OBJECT_SCALE:
    {
        MWWorld::CellStore *ptrCellStore = Main::get().getWorldController()->getCell(event->cell);

        if (!ptrCellStore) return;

        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "%s", "Received ID_OBJECT_SCALE");
        LOG_APPEND(Log::LOG_WARN, "- cellRef: %s, %i\n- cell: %s",
            event->cellRef.mRefID.c_str(),
            event->cellRef.mRefNum.mIndex,
            event->cell.getDescription().c_str());

        MWWorld::Ptr ptrFound = ptrCellStore->searchExact(event->cellRef.mRefID, event->cellRef.mRefNum.mIndex);

        if (ptrFound)
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Found %s, %i",
                ptrFound.getCellRef().getRefId().c_str(),
                ptrFound.getCellRef().getRefNum());

            MWBase::Environment::get().getWorld()->scaleObject(ptrFound, event->scale);
        }

        break;
    }
    case ID_OBJECT_MOVE:
    {
        MWWorld::CellStore *ptrCellStore = Main::get().getWorldController()->getCell(event->cell);

        if (!ptrCellStore) return;

        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_OBJECT_MOVE");
        LOG_APPEND(Log::LOG_WARN, "- cellRef: %s, %i\n- cell: %s",
            event->cellRef.mRefID.c_str(),
            event->cellRef.mRefNum.mIndex,
            event->cell.getDescription().c_str());

        MWWorld::Ptr ptrFound = ptrCellStore->searchExact(event->cellRef.mRefID, event->cellRef.mRefNum.mIndex);

        if (ptrFound)
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Found %s, %i",
                ptrFound.getCellRef().getRefId().c_str(),
                ptrFound.getCellRef().getRefNum());

            MWBase::Environment::get().getWorld()->moveObject(ptrFound,
                event->pos.pos[0], event->pos.pos[1], event->pos.pos[2]);
        }

        break;
    }
    case ID_OBJECT_ROTATE:
    {
        MWWorld::CellStore *ptrCellStore = Main::get().getWorldController()->getCell(event->cell);

        if (!ptrCellStore) return;

        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_OBJECT_ROTATE");
        LOG_APPEND(Log::LOG_WARN, "- cellRef: %s, %i\n- cell: %s",
            event->cellRef.mRefID.c_str(),
            event->cellRef.mRefNum.mIndex,
            event->cell.getDescription().c_str());

        MWWorld::Ptr ptrFound = ptrCellStore->searchExact(event->cellRef.mRefID, event->cellRef.mRefNum.mIndex);

        if (ptrFound)
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Found %s, %i",
                ptrFound.getCellRef().getRefId().c_str(),
                ptrFound.getCellRef().getRefNum());

            MWBase::Environment::get().getWorld()->rotateObject(ptrFound,
                event->pos.rot[0], event->pos.rot[1], event->pos.rot[2]);
        }

        break;
    }
    case ID_OBJECT_ANIM_PLAY:
    {
        MWWorld::CellStore *ptrCellStore = Main::get().getWorldController()->getCell(event->cell);

        if (!ptrCellStore) return;

        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_OBJECT_ANIM_PLAY");
        LOG_APPEND(Log::LOG_WARN, "- cellRef: %s, %i\n- cell: %s",
            event->cellRef.mRefID.c_str(),
            event->cellRef.mRefNum.mIndex,
            event->cell.getDescription().c_str());

        MWWorld::Ptr ptrFound = ptrCellStore->searchExact(event->cellRef.mRefID, event->cellRef.mRefNum.mIndex);

        if (ptrFound)
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Found %s, %i",
                ptrFound.getCellRef().getRefId().c_str(),
                ptrFound.getCellRef().getRefNum());

            MWBase::Environment::get().getMechanicsManager()->playAnimationGroup(ptrFound,
                event->animGroup, event->animMode, std::numeric_limits<int>::max(), true);
        }

        break;
    }
    case ID_DOOR_ACTIVATE:
    {
        MWWorld::CellStore *ptrCellStore = Main::get().getWorldController()->getCell(event->cell);

        if (!ptrCellStore) return;

        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_DOOR_ACTIVATE");
        LOG_APPEND(Log::LOG_WARN, "- cellRef: %s, %i\n- cell: %s",
            event->cellRef.mRefID.c_str(),
            event->cellRef.mRefNum.mIndex,
            event->cell.getDescription().c_str());

        MWWorld::Ptr ptrFound = ptrCellStore->searchExact(event->cellRef.mRefID, event->cellRef.mRefNum.mIndex);

        if (ptrFound)
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Found %s, %i",
                ptrFound.getCellRef().getRefId().c_str(),
                ptrFound.getCellRef().getRefNum());

            ptrFound.getClass().setDoorState(ptrFound, event->state);
            MWBase::Environment::get().getWorld()->saveDoorState(ptrFound, event->state);
        }

        break;
    }
    case ID_VIDEO_PLAY:
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_VIDEO_PLAY");
        LOG_APPEND(Log::LOG_WARN, "- video: %s\n- allowSkipping: %s",
            event->video.c_str(),
            event->allowSkipping ? "true" : "false");

        MWBase::Environment::get().getWindowManager()->playVideo(event->video, event->allowSkipping);

        break;
    }
    case ID_SCRIPT_LOCAL_SHORT:
    {
        MWWorld::CellStore *ptrCellStore = Main::get().getWorldController()->getCell(event->cell);

        if (!ptrCellStore) return;

        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_SCRIPT_LOCAL_SHORT");
        LOG_APPEND(Log::LOG_WARN, "- cellRef: %s, %i\n- cell: %s\n- index: %i\n- shortVal: %i",
            event->cellRef.mRefID.c_str(),
            event->cellRef.mRefNum.mIndex,
            event->cell.getDescription().c_str(),
            event->index,
            event->shortVal);

        MWWorld::Ptr ptrFound = ptrCellStore->searchExact(event->cellRef.mRefID, event->cellRef.mRefNum.mIndex);

        if (ptrFound)
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Found %s, %i",
                ptrFound.getCellRef().getRefId().c_str(),
                ptrFound.getCellRef().getRefNum());

            ptrFound.getRefData().getLocals().mShorts.at(event->index) = event->shortVal;
        }

        break;
    }
    case ID_SCRIPT_LOCAL_FLOAT:
    {
        MWWorld::CellStore *ptrCellStore = Main::get().getWorldController()->getCell(event->cell);

        if (!ptrCellStore) return;

        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_SCRIPT_LOCAL_FLOAT");
        LOG_APPEND(Log::LOG_WARN, "- cellRef: %s, %i\n- cell: %s\n- index: %i\n- floatVal: %f",
            event->cellRef.mRefID.c_str(),
            event->cellRef.mRefNum.mIndex,
            event->cell.getDescription().c_str(),
            event->index,
            event->floatVal);

        MWWorld::Ptr ptrFound = ptrCellStore->searchExact(event->cellRef.mRefID, event->cellRef.mRefNum.mIndex);

        if (ptrFound)
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Found %s, %i",
                ptrFound.getCellRef().getRefId().c_str(),
                ptrFound.getCellRef().getRefNum());

            ptrFound.getRefData().getLocals().mFloats.at(event->index) = event->floatVal;
        }

        break;
    }
    case ID_SCRIPT_MEMBER_SHORT:
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_SCRIPT_MEMBER_SHORT");
        LOG_APPEND(Log::LOG_WARN, "- cellRef: %s\n- index: %i\n- shortVal: %i\n",
            event->cellRef.mRefID.c_str(),
            event->index,
            event->shortVal);

        // Mimic the way a Ptr is fetched in InterpreterContext for similar situations
        MWWorld::Ptr ptrFound = MWBase::Environment::get().getWorld()->getPtr(event->cellRef.mRefID, false);

        if (ptrFound)
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Found %s, %i",
                ptrFound.getCellRef().getRefId().c_str(),
                ptrFound.getCellRef().getRefNum());

            std::string scriptId = ptrFound.getClass().getScript(ptrFound);

            ptrFound.getRefData().setLocals(
                *MWBase::Environment::get().getWorld()->getStore().get<ESM::Script>().find(scriptId));

            ptrFound.getRefData().getLocals().mShorts.at(event->index) = event->shortVal;;
        }

        break;
    }
    case ID_SCRIPT_GLOBAL_SHORT:
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Received ID_SCRIPT_GLOBAL_SHORT");
        LOG_APPEND(Log::LOG_WARN, "- varName: %s\n- shortVal: %i",
            event->varName.c_str(),
            event->shortVal);

        MWBase::Environment::get().getWorld()->setGlobalInt(event->varName, event->shortVal);

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

WorldEvent *Networking::createWorldEvent()
{
    return new WorldEvent(getLocalPlayer()->guid);
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

    return pl->getAttack()->pressed;
}

bool Networking::isConnected()
{
    return connected;
}
