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
#include "Networking.hpp"
#include "../mwstate/statemanagerimp.hpp"
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
            case ID_CONNECTION_REQUEST_ACCEPTED:
                printf("Our connection request has been accepted.\n");
                break;
            case ID_NEW_INCOMING_CONNECTION:
                printf("A connection is incoming.\n");
                break;
            case ID_NO_FREE_INCOMING_CONNECTIONS:
                printf("The server is full.\n");
                MWBase::Environment::get().getStateManager()->requestQuit();
                break;
            case ID_DISCONNECTION_NOTIFICATION:
                    printf("We have been disconnected.\n");
                    MWBase::Environment::get().getStateManager()->requestQuit();
                break;
            case ID_CONNECTION_LOST:
                    printf("Connection lost.\n");
                    MWBase::Environment::get().getStateManager()->requestQuit();
                break;
            case ID_CUSTOM_MESSAGE:
                ReciveMessage(packet);
                break;
            default:
                printf("Message with identifier %i has arrived.\n", packet->data[0]);
                break;
        }
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
    const char passw[8] = "1234567";


    if (peer->Connect(master.ToString(false), master.GetPort(), passw, sizeof(passw), 0, 0, 3, 500, 0) != RakNet::CONNECTION_ATTEMPT_STARTED)
    {
        cout << "Connection attempt failed." << endl;
        MWBase::Environment::get().getStateManager()->requestQuit();
    }

    bool queue = true;
    while(queue)
    {
        for (RakNet::Packet *packet = peer->Receive(); packet; peer->DeallocatePacket(
                packet), packet = peer->Receive())
        {
            switch (packet->data[0])
            {
                case ID_CONNECTION_ATTEMPT_FAILED:
                {
                    cerr << "Connection failed." << endl;
                    MWBase::Environment::get().getStateManager()->requestQuit();
                    queue = false;
                    break;
                }
                case ID_INVALID_PASSWORD:
                {
                    cerr << "Connection failed. Different versions of client or server." << endl
                         << "Ask your server administrator for resolve this problem." << endl;
                    MWBase::Environment::get().getStateManager()->requestQuit();
                    queue = false;
                    break;
                }
                case ID_CONNECTION_REQUEST_ACCEPTED:
                {
                    serverAddr = packet->systemAddress;
                    connected = true;
                    queue = false;

                    GetPacket(ID_GAME_BASE_INFO)->Send(getLocalPlayer());

                    break;
                }
                case ID_DISCONNECTION_NOTIFICATION:
                    throw runtime_error("ID_DISCONNECTION_NOTIFICATION.\n");
                case ID_CONNECTION_BANNED:
                    throw runtime_error("ID_CONNECTION_BANNED.\n");
                case ID_CONNECTION_LOST:
                    throw runtime_error("ID_CONNECTION_LOST.\n");
                default:
                    printf("Connection message with identifier %i has arrived in initialization.\n", packet->data[0]);
            }
        }
    }
}

void Networking::ReciveMessage(RakNet::Packet *packet)
{
    RakNet::RakNetGUID id;

    if(packet->length < 3)
        return;

    RakNet::BitStream bsIn(&packet->data[2], packet->length, false);
    bsIn.Read(id);

    DedicatedPlayer *pl = 0;
    static RakNet::RakNetGUID myid = getLocalPlayer()->guid;
    if(id != myid)
        pl = Players::GetPlayer(id);

    BasePacket *myPacket = controller.GetPacket(packet->data[1]);

    switch(packet->data[1])
    {
        case ID_HANDSHAKE:
        {
            (*getLocalPlayer()->GetPassw()) = "SuperPassword";
            myPacket->Send(getLocalPlayer(), serverAddr);
            break;
        }
        case ID_GAME_BASE_INFO:
        {
            if(id == myid)
            {
                cout << "TEST: " << packet->length << endl;
                if(packet->length == myPacket->headerSize())
                {
                    cout << "ID_GAME_BASE_INFO request only" << endl;
                    myPacket->Send(getLocalPlayer(), serverAddr);
                }
                else
                {
                    myPacket->Packet(&bsIn, getLocalPlayer(), false);
                    cout << "ID_GAME_BASE_INFO" << endl;
                    getLocalPlayer()->updateChar();
                }
            }
            else
            {
                if(pl == 0)
                    pl = Players::NewPlayer(id);

                myPacket->Packet(&bsIn, pl, false);
                Players::CreatePlayer(id);
            }
            break;
        }
        case ID_GAME_UPDATE_POS:
        {
            if(id == myid)
            {
                if (packet->length != myPacket->headerSize())
                {
                    cout << "ID_GAME_UPDATE_POS changed by server" << endl;
                    myPacket->Packet(&bsIn, getLocalPlayer(), false);
                    getLocalPlayer()->setPosition();
                }
                else
                    getLocalPlayer()->updatePosition(true);
            }
            else if(pl != 0)
                myPacket->Packet(&bsIn, pl, false);
            break;
        }
        case ID_USER_MYID:
        {
            cout << "ID_USER_MYID" << endl;
            myid = id;
            getLocalPlayer()->guid = id;
            break;
        }
        case ID_USER_DISCONNECTED:
        {
            if(id == myid)
                MWBase::Environment::get().getStateManager()->requestQuit();
            else if(pl != 0)
                Players::DisconnectPlayer(id);

        }
        case ID_GAME_UPDATE_EQUIPED:
        {
            if(id == myid)
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
        case ID_GAME_UPDATE_SKILLS:
        {
            if(id == myid)
            {
                getLocalPlayer()->updateAttributesAndSkills(true);
                myPacket->Send(getLocalPlayer(), serverAddr);
            }
            else if (pl != 0)
            {
                myPacket->Packet(&bsIn, pl, false);

                MWMechanics::SkillValue skillValue;
                MWMechanics::AttributeValue attributeValue;
                for (int i = 0; i < PacketAttributesAndStats::StatsCount; ++i)
                {
                    skillValue.readState(pl->NpcStats()->mSkills[i]);
                    pl->getPtr().getClass().getNpcStats(pl->getPtr()).setSkill(i, skillValue);
                }

                for (int i = 0; i < PacketAttributesAndStats::AttributesCount; ++i)
                {
                    attributeValue.readState(pl->CreatureStats()->mAttributes[i]);
                    pl->getPtr().getClass().getCreatureStats(pl->getPtr()).setAttribute(i, attributeValue);
                }
            }
            break;
        }
        case ID_GAME_ATTACK:
        {
            if(pl != 0)
            {
                myPacket->Packet(&bsIn, pl, false);

                //cout << "Player: " << pl->Npc()->mName << " pressed: " << (pl->GetAttack()->pressed == 1) << endl;
                if(pl->GetAttack()->pressed == 0)
                {
                    cout << "success: " << (pl->GetAttack()->success == 1);
                    if(pl->GetAttack()->success == 1)
                        cout << " damage: " << pl->GetAttack()->damage;
                    cout << endl;
                }

                MWMechanics::CreatureStats &stats = pl->getPtr().getClass().getNpcStats(pl->getPtr());
                stats.getSpells().setSelectedSpell(pl->GetAttack()->refid);

                MWWorld::Ptr victim;
                if(pl->GetAttack()->target == getLocalPlayer()->guid)
                    victim = MWBase::Environment::get().getWorld()->getPlayerPtr();
                else if(Players::GetPlayer(pl->GetAttack()->target) != 0)
                    victim = Players::GetPlayer(pl->GetAttack()->target)->getPtr();

                MWWorld::Ptr attacker;
                attacker = pl->getPtr();

                // Get the weapon used (if hand-to-hand, weapon = inv.end())
                if(*pl->DrawState() == 1)
                {
                    MWWorld::InventoryStore &inv = attacker.getClass().getInventoryStore(attacker);
                    MWWorld::ContainerStoreIterator weaponslot = inv.getSlot(
                            MWWorld::InventoryStore::Slot_CarriedRight);
                    MWWorld::Ptr weapon = ((weaponslot != inv.end()) ? *weaponslot : MWWorld::Ptr());
                    if (!weapon.isEmpty() && weapon.getTypeName() != typeid(ESM::Weapon).name())
                        weapon = MWWorld::Ptr();

                    bool healthdmg;
                    if (!weapon.isEmpty())
                        healthdmg = true;
                    else
                    {
                        MWMechanics::CreatureStats &otherstats = victim.getClass().getCreatureStats(victim);
                        healthdmg = otherstats.isParalyzed() || otherstats.getKnockedDown();
                    }

                    if (victim.mRef != 0)
                    {
                        if (!weapon.isEmpty())
                            MWMechanics::blockMeleeAttack(attacker, victim, weapon, pl->GetAttack()->damage, 1);
                        pl->getPtr().getClass().onHit(victim, pl->GetAttack()->damage, healthdmg, weapon, attacker,
                                                      pl->GetAttack()->success);
                    }
                }
                else
                {
                    cout << "SpellId: " << pl->GetAttack()->refid << endl;
                }
            }
            break;
        }
        case ID_GAME_UPDATE_BASESTATS:
        {
            if(id == myid)
            {
                getLocalPlayer()->updateBaseStats(true);
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
            printf("ID_GAME_DIE\n");
            if(id == myid)
            {
                MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
                MWMechanics::DynamicStat<float> health = player.getClass().getCreatureStats(player).getHealth();
                health.setCurrent(0);
                player.getClass().getCreatureStats(player).setHealth(health);
                myPacket->Send(getLocalPlayer(), serverAddr);
            }
            else if(pl != 0)
            {
                printf("attempt to kill %s\n", pl->Npc()->mName.c_str());
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
                MWBase::Environment::get().getWorld()->findInteriorPosition("ToddTest", pos);
                MWBase::Environment::get().getWorld()->changeToInteriorCell("ToddTest", pos, true);
                (*getLocalPlayer()->Position()) = pos;
                (*getLocalPlayer()->GetCell()) = *player.getCell()->getCell();
                myPacket->Send(getLocalPlayer(), serverAddr);

                getLocalPlayer()->updateBaseStats(true);
                controller.GetPacket(ID_GAME_UPDATE_BASESTATS)->Send(getLocalPlayer(), serverAddr);
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
                if(id == myid)
                {
                    myPacket->Packet(&bsIn, getLocalPlayer(), false);

                    if(!getLocalPlayer()->GetCell()->isExterior())
                    {
                        cout << "location: " << getLocalPlayer()->GetCell()->mName << endl;

                        MWBase::World *world = MWBase::Environment::get().getWorld();
                        ESM::Position pos;
                        world->findInteriorPosition(getLocalPlayer()->GetCell()->mName, pos);
                        world->changeToInteriorCell(getLocalPlayer()->GetCell()->mName, pos, true);
                    }

                    getLocalPlayer()->updateCell(true);
                }
                else if(pl != 0)
                {
                    myPacket->Packet(&bsIn, pl, false);
                    pl->updateCell();
                }
                break;
            }
            case ID_GAME_DRAWSTATE:
            {
                if(id == myid)
                    getLocalPlayer()->updateDrawStateAndFlags(true);
                else if(pl != 0)
                {
                    myPacket->Packet(&bsIn, pl, false);
                    pl->UpdateDrawState();
                }
                break;
            }
            case ID_CHAT_MESSAGE:
            {
                std::string message;
                if(id == myid)
                {
                    myPacket->Packet(&bsIn, getLocalPlayer(), false);
                    message =  *getLocalPlayer()->ChatMessage();
                }
                else if(pl != 0)
                {
                    myPacket->Packet(&bsIn, pl, false);
                    message =  *pl->ChatMessage();
                }
                Main::get().getGUIConroller()->PrintChatMessage(message);
                break;
            }
            case ID_GAME_CHARGEN:
            {
                if(id == myid)
                {
                    myPacket->Packet(&bsIn, getLocalPlayer(), false);
                }
                break;
            }

            case ID_GAME_ATTRIBUTE:
            {
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

                MWMechanics::AttributeValue attributeValue;

                myPacket->Packet(&bsIn, __pl, false);

                for (int i = 0; i < PacketAttributesAndStats::AttributesCount; ++i)
                {
                    attributeValue.readState(__pl->CreatureStats()->mAttributes[i]);
                    __pl_ptr.getClass().getCreatureStats(__pl_ptr).setAttribute(i, attributeValue);
                }
                break;
            }

            case ID_GAME_SKILL:
            {
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

                MWMechanics::SkillValue skillValue;

                myPacket->Packet(&bsIn, __pl, false);

                for (int i = 0; i < PacketAttributesAndStats::StatsCount; ++i)
                {
                    skillValue.readState(__pl->NpcStats()->mSkills[i]);
                    __pl_ptr.getClass().getNpcStats(__pl_ptr).setSkill(i, skillValue);
                    //printf("skill %d, value %d\n", i, skillValue.getBase());
                }

                break;
            }
            case ID_GUI_MESSAGEBOX:
            {
                if(id == myid)
                {
                    myPacket->Packet(&bsIn, getLocalPlayer(), false);

                    printf("ID_GUI_MESSAGEBOX, Type %d, MSG %s\n", getLocalPlayer()->guiMessageBox.type, getLocalPlayer()->guiMessageBox.label.c_str());

                    if(getLocalPlayer()->guiMessageBox.type == BasePlayer::GUIMessageBox::MessageBox)
                        Main::get().getGUIConroller()->ShowMessageBox(getLocalPlayer()->guiMessageBox);
                    else if(getLocalPlayer()->guiMessageBox.type == BasePlayer::GUIMessageBox::CustomMessageBox)
                        Main::get().getGUIConroller()->ShowCustomMessageBox(getLocalPlayer()->guiMessageBox);
                    else if(getLocalPlayer()->guiMessageBox.type == BasePlayer::GUIMessageBox::InputDialog)
                        Main::get().getGUIConroller()->ShowInputBox(getLocalPlayer()->guiMessageBox);
                }
                break;
            }
        default:
            printf("Custom message with identifier %i has arrived in initialization.\n", packet->data[1]);
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
    if(ptr.mRef == 0)
        return 0;
    DedicatedPlayer *pl = Players::GetPlayer(ptr);

    return pl != 0;
}

bool Networking::Attack(const MWWorld::Ptr &ptr)
{
    DedicatedPlayer *pl = Players::GetPlayer(ptr);

    if(pl == 0)
        return false;

    return pl->GetAttack()->pressed;
}

bool Networking::isConnected()
{
    return connected;
}
