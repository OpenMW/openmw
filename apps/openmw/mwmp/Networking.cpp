//
// Created by koncord on 04.01.16.
//

#include <stdexcept>
#include <iostream>
#include <string>
#include <components/esm/cellid.hpp>
#include <apps/openmw/mwbase/world.hpp>
#include <apps/openmw/mwbase/environment.hpp>
#include <apps/openmw/mwworld/cellstore.hpp>
#include <apps/openmw/mwclass/npc.hpp>
#include <apps/openmw/mwmechanics/npcstats.hpp>
#include <apps/openmw/mwworld/inventorystore.hpp>
#include <apps/openmw/mwmechanics/combat.hpp>
#include <SDL_messagebox.h>
#include "Networking.hpp"
#include "../mwstate/statemanagerimp.hpp"
#include <components/openmw-mp/Log.hpp>
#include <components/openmw-mp/Version.hpp>
#include "DedicatedPlayer.hpp"
#include "Main.hpp"

using namespace std;
using namespace mwmp;

Networking::Networking(): peer(RakNet::RakPeerInterface::GetInstance()), controller(peer)
{

    RakNet::SocketDescriptor sd;
    sd.port=0;
    RakNet::StartupResult b = peer->Startup(1,&sd, 1);
    RakAssert(b==RAKNET_STARTED);

    controller.SetStream(0, &bsOut);
    connected = 0;
}

Networking::~Networking()
{
    peer->Shutdown(100);
    peer->CloseConnection(peer->GetSystemAddressFromIndex(0), true, 0);
    RakNet::RakPeerInterface::DestroyInstance(peer);
}

void Networking::Update()
{
    RakNet::Packet *packet;
    std::string errmsg = "";

    for (packet=peer->Receive(); packet; peer->DeallocatePacket(packet), packet=peer->Receive())
    {
        switch (packet->data[0])
        {
            case ID_REMOTE_DISCONNECTION_NOTIFICATION:
                LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "%s", "Another client has disconnected.");
                break;
            case ID_REMOTE_CONNECTION_LOST:
                LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "%s", "Another client has lost connection.");
                break;
            case ID_REMOTE_NEW_INCOMING_CONNECTION:
                LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "%s", "Another client has connected.");
                break;
            case ID_CONNECTION_REQUEST_ACCEPTED:
                LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "%s", "Our connection request has been accepted.");
                break;
            case ID_NEW_INCOMING_CONNECTION:
                LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "%s", "A connection is incoming.");
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
                ReceiveMessage(packet);
                //LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Message with identifier %i has arrived.", packet->data[0]);
                break;
        }
    }

    if (!errmsg.empty())
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_ERROR, "%s", errmsg.c_str());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "tes3mp", errmsg.c_str(), 0);
        MWBase::Environment::get().getStateManager()->requestQuit();
    }
}

void Networking::SendData(RakNet::BitStream *bs)
{
    peer->Send(bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, serverAddr, false);
}

void Networking::Connect(const std::string &ip, unsigned short port)
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
        LOG_MESSAGE_SIMPLE(Log::LOG_ERROR, "%s", errmsg.c_str());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "tes3mp", errmsg.c_str(), 0);
        MWBase::Environment::get().getStateManager()->requestQuit();
    }
}

void Networking::ReceiveMessage(RakNet::Packet *packet)
{
    RakNet::RakNetGUID id;

    if (packet->length < 2)
        return;

    RakNet::BitStream bsIn(&packet->data[1], packet->length, false);
    bsIn.Read(id);

    DedicatedPlayer *pl = 0;
    static RakNet::RakNetGUID myid = getLocalPlayer()->guid;
    if (id != myid)
        pl = Players::GetPlayer(id);

    BasePacket *myPacket = controller.GetPacket(packet->data[0]);

    switch (packet->data[0])
    {
        case ID_HANDSHAKE:
        {
            (*getLocalPlayer()->GetPassw()) = "SuperPassword";
            myPacket->Send(getLocalPlayer(), serverAddr);
            break;
        }
        case ID_GAME_BASE_INFO:
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "%s", "Received ID_GAME_BASE_INFO from server");

            if (id == myid)
            {
                LOG_APPEND(Log::LOG_INFO, "%s", "- Packet was about my id");

                if (packet->length == myPacket->headerSize())
                {
                    LOG_APPEND(Log::LOG_INFO, "%s", "- Requesting info");
                    myPacket->Send(getLocalPlayer(), serverAddr);
                }
                else
                {
                    myPacket->Packet(&bsIn, getLocalPlayer(), false);
                    LOG_APPEND(Log::LOG_INFO, "%s", "- Updating LocalPlayer");
                    getLocalPlayer()->updateChar();
                }
            }
            else
            {
                LOG_APPEND(Log::LOG_INFO, "- Packet was about %s", pl == 0 ? "new player" : pl->Npc()->mName.c_str());

                if (pl == 0)
                {
                    LOG_APPEND(Log::LOG_INFO, "%s", "- Exchanging data with new player");
                    pl = Players::NewPlayer(id);
                }

                myPacket->Packet(&bsIn, pl, false);
                Players::CreatePlayer(id);
            }
            break;
        }
        case ID_GAME_POS:
        {
            if (id == myid)
            {
                if (packet->length != myPacket->headerSize())
                {
                    LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "%s", "ID_GAME_POS changed by server");
                    myPacket->Packet(&bsIn, getLocalPlayer(), false);
                    getLocalPlayer()->setPosition();
                }
                else
                    getLocalPlayer()->updatePosition(true);
            }
            else if (pl != 0)
                myPacket->Packet(&bsIn, pl, false);
            break;
        }
        case ID_USER_MYID:
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "%s", "Received ID_USER_MYID from server");
            myid = id;
            getLocalPlayer()->guid = id;
            break;
        }
        case ID_USER_DISCONNECTED:
        {
            if (id == myid)
                MWBase::Environment::get().getStateManager()->requestQuit();
            else if (pl != 0)
                Players::DisconnectPlayer(id);

        }
        case ID_GAME_EQUIPMENT:
        {
            if (id == myid)
            {
                getLocalPlayer()->updateInventory(true);
                myPacket->Send(getLocalPlayer(), serverAddr);
            }
            else if (pl != 0)
            {
                myPacket->Packet(&bsIn, pl, false);
                pl->UpdateInventory();
            }
            break;
        }
        case ID_GAME_ATTACK:
        {
            if (pl != 0)
            {
                myPacket->Packet(&bsIn, pl, false);

                //cout << "Player: " << pl->Npc()->mName << " pressed: " << (pl->GetAttack()->pressed == 1) << endl;
                if (pl->GetAttack()->pressed == 0)
                {
                    LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Attack success: %s",
                        pl->GetAttack()->success ? "true" : "false");

                    if (pl->GetAttack()->success == 1)
                    {
                        LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Damage: %f",
                            pl->GetAttack()->damage);
                    }
                }

                MWMechanics::CreatureStats &stats = pl->getPtr().getClass().getNpcStats(pl->getPtr());
                stats.getSpells().setSelectedSpell(pl->GetAttack()->refid);

                MWWorld::Ptr victim;
                if (pl->GetAttack()->target == getLocalPlayer()->guid)
                    victim = MWBase::Environment::get().getWorld()->getPlayerPtr();
                else if (Players::GetPlayer(pl->GetAttack()->target) != 0)
                    victim = Players::GetPlayer(pl->GetAttack()->target)->getPtr();

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
                            MWMechanics::blockMeleeAttack(attacker, victim, weapon, pl->GetAttack()->damage, 1);
                        pl->getPtr().getClass().onHit(victim, pl->GetAttack()->damage, healthdmg, weapon, attacker, osg::Vec3f(),
                                                      pl->GetAttack()->success);
                    }
                }
                else
                {
                    LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "SpellId: %s",
                        pl->GetAttack()->refid.c_str());
                }
            }
            break;
        }
        case ID_GAME_DYNAMICSTATS:
        {
            if (id == myid)
            {
                getLocalPlayer()->updateDynamicStats(true);
                myPacket->Send(getLocalPlayer(), serverAddr);
            }
            else if (pl != 0)
            {
                myPacket->Packet(&bsIn, pl, false);
                MWMechanics::DynamicStat<float> value;

                value.readState(pl->CreatureStats()->mDynamic[0]);
                pl->getPtr().getClass().getCreatureStats(pl->getPtr()).setHealth(value);
                value.readState(pl->CreatureStats()->mDynamic[1]);
                pl->getPtr().getClass().getCreatureStats(pl->getPtr()).setMagicka(value);
                value.readState(pl->CreatureStats()->mDynamic[2]);
                pl->getPtr().getClass().getCreatureStats(pl->getPtr()).setFatigue(value);
            }
            break;
        }
        case ID_GAME_DIE:
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "%s", "Received ID_GAME_DIE from server");
            if (id == myid)
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
            if (id == myid)
            {
                MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
                player.getClass().getCreatureStats(player).resurrect();
                ESM::Position pos;
                MWBase::Environment::get().getWorld()->findInteriorPosition("Pelagiad, Fort Pelagiad", pos);
                MWBase::Environment::get().getWorld()->changeToInteriorCell("Pelagiad, Fort Pelagiad", pos, true);
                (*getLocalPlayer()->Position()) = pos;
                (*getLocalPlayer()->GetCell()) = *player.getCell()->getCell();
                myPacket->Send(getLocalPlayer(), serverAddr);

                getLocalPlayer()->updateDynamicStats(true);
                controller.GetPacket(ID_GAME_DYNAMICSTATS)->Send(getLocalPlayer(), serverAddr);
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
                if (id == myid)
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
                if (id == myid)
                    getLocalPlayer()->updateDrawStateAndFlags(true);
                else if (pl != 0)
                {
                    myPacket->Packet(&bsIn, pl, false);
                    pl->UpdateDrawState();
                }
                break;
            }
            case ID_CHAT_MESSAGE:
            {
                std::string message;
                if (id == myid)
                {
                    myPacket->Packet(&bsIn, getLocalPlayer(), false);
                    message =  *getLocalPlayer()->ChatMessage();
                }
                else if (pl != 0)
                {
                    myPacket->Packet(&bsIn, pl, false);
                    message =  *pl->ChatMessage();
                }
                Main::get().getGUIController()->PrintChatMessage(message);
                break;
            }
            case ID_GAME_CHARGEN:
            {
                if (id == myid)
                {
                    myPacket->Packet(&bsIn, getLocalPlayer(), false);
                }
                break;
            }

            case ID_GAME_ATTRIBUTE:
            {
                // Ignore requests for this packet
                if (packet->length == myPacket->headerSize())
                    return;

                BasePlayer *__pl = nullptr;
                MWWorld::Ptr __pl_ptr;
                if (id == myid)
                {
                    __pl = getLocalPlayer();
                    __pl_ptr = MWBase::Environment::get().getWorld()->getPlayerPtr();
                }
                else if (pl != 0)
                {
                    __pl = pl;
                    __pl_ptr = pl->getPtr();
                }
                else
                    return;

                myPacket->Packet(&bsIn, __pl, false);

                MWMechanics::AttributeValue attributeValue;

                for (int i = 0; i < 8; ++i)
                {
                    attributeValue.readState(__pl->CreatureStats()->mAttributes[i]);
                    __pl_ptr.getClass().getCreatureStats(__pl_ptr).setAttribute(i, attributeValue);
                }
                break;
            }

            case ID_GAME_SKILL:
            {
                // Ignore requests for this packet
                if (packet->length == myPacket->headerSize())
                    return;

                BasePlayer *__pl = nullptr;
                MWWorld::Ptr __pl_ptr;
                if (id == myid)
                {
                    __pl = getLocalPlayer();
                    __pl_ptr = MWBase::Environment::get().getWorld()->getPlayerPtr();
                }
                else if (pl != 0)
                {
                    __pl = pl;
                    __pl_ptr = pl->getPtr();
                }
                else
                    return;

                myPacket->Packet(&bsIn, __pl, false);

                MWMechanics::SkillValue skillValue;

                for (int i = 0; i < 27; ++i)
                {
                    skillValue.readState(__pl->NpcStats()->mSkills[i]);
                    __pl_ptr.getClass().getNpcStats(__pl_ptr).setSkill(i, skillValue);
                    //LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "skill %d, value %d", i, skillValue.getBase());
                }

                break;
            }

            case ID_GAME_LEVEL:
            {
                // Ignore requests for this packet
                if (packet->length == myPacket->headerSize())
                    return;

                BasePlayer *__pl = nullptr;
                MWWorld::Ptr __pl_ptr;
                if (id == myid)
                {
                    __pl = getLocalPlayer();
                    __pl_ptr = MWBase::Environment::get().getWorld()->getPlayerPtr();
                }
                else if (pl != 0)
                {
                    __pl = pl;
                    __pl_ptr = pl->getPtr();
                }
                else
                    return;

                myPacket->Packet(&bsIn, __pl, false);

                __pl_ptr.getClass().getCreatureStats(__pl_ptr).setLevel(__pl->CreatureStats()->mLevel);

                break;
            }

            case ID_GUI_MESSAGEBOX:
            {
                if (id == myid)
                {
                    myPacket->Packet(&bsIn, getLocalPlayer(), false);

                    LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "ID_GUI_MESSAGEBOX, Type %d, MSG %s",
                        getLocalPlayer()->guiMessageBox.type,
                        getLocalPlayer()->guiMessageBox.label.c_str());

                    if (getLocalPlayer()->guiMessageBox.type == BasePlayer::GUIMessageBox::MessageBox)
                        Main::get().getGUIController()->ShowMessageBox(getLocalPlayer()->guiMessageBox);
                    else if (getLocalPlayer()->guiMessageBox.type == BasePlayer::GUIMessageBox::CustomMessageBox)
                        Main::get().getGUIController()->ShowCustomMessageBox(getLocalPlayer()->guiMessageBox);
                    else if (getLocalPlayer()->guiMessageBox.type == BasePlayer::GUIMessageBox::InputDialog)
                        Main::get().getGUIController()->ShowInputBox(getLocalPlayer()->guiMessageBox);
                }
                break;
            }
        case ID_GAME_CHARCLASS:
        {
            if (id == myid)
            {
                if (packet->length == myPacket->headerSize())
                    getLocalPlayer()->SendClass();
                else
                {
                    myPacket->Packet(&bsIn, getLocalPlayer(), false);
                    getLocalPlayer()->SetClass();
                }
            }
            break;
        }
        case ID_GAME_TIME:
        {
            if (id == myid)
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
        }
        default:
            LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Custom message with identifier %i has arrived in initialization.", packet->data[0]);
    }
}

BasePacket *Networking::GetPacket(RakNet::MessageID id)
{
    return controller.GetPacket(id);
}

LocalPlayer *Networking::getLocalPlayer()
{
    return mwmp::Main::get().getLocalPlayer();
}

bool Networking::isDedicatedPlayer(const MWWorld::Ptr &ptr)
{
    if (ptr.mRef == 0)
        return 0;
    DedicatedPlayer *pl = Players::GetPlayer(ptr);

    return pl != 0;
}

bool Networking::Attack(const MWWorld::Ptr &ptr)
{
    DedicatedPlayer *pl = Players::GetPlayer(ptr);

    if (pl == 0)
        return false;

    return pl->GetAttack()->pressed;
}

bool Networking::isConnected()
{
    return connected;
}
