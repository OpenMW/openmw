//
// Created by koncord on 14.01.16.
//

#include <apps/openmw/mwworld/manualref.hpp>
#include <apps/openmw/mwmechanics/aitravel.hpp>
#include <components/esm/esmwriter.hpp>
#include "../mwbase/environment.hpp"
#include "../mwstate/statemanagerimp.hpp"
#include "../mwinput/inputmanagerimp.hpp"
#include "../mwscript/scriptmanagerimp.hpp"
#include "../mwgui/windowmanagerimp.hpp"
#include "../mwworld/worldimp.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/customdata.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwclass/npc.hpp"
#include "../mwclass/creature.hpp"
#include "../mwmechanics/mechanicsmanagerimp.hpp"
#include "../mwworld/cellstore.hpp"
#include <apps/openmw/mwdialogue/dialoguemanagerimp.hpp>
#include <apps/openmw/mwworld/inventorystore.hpp>
#include <apps/openmw/mwmechanics/spellcasting.hpp>
#include <apps/openmw/mwgui/inventorywindow.hpp>

#include "LocalPlayer.hpp"
#include "Main.hpp"

using namespace mwmp;
using namespace std;

LocalPlayer::LocalPlayer()
{
    CharGenStage()->current = 0;
    CharGenStage()->end = 1;
}

LocalPlayer::~LocalPlayer()
{

}

void LocalPlayer::Update()
{
    updateCell();
    updatePosition();
    updateDrawStateAndFlags();
    updateAttackState();
    updateDeadState();
    updateInventory();
    updateBaseStats();
}

MWWorld::Ptr LocalPlayer::GetPlayerPtr()
{
    return MWBase::Environment::get().getWorld()->getPlayerPtr();
}

void LocalPlayer::updateBaseStats(bool forceUpdate)
{
    MWWorld::Ptr player = GetPlayerPtr();

    MWMechanics::CreatureStats *creatureClass = &player.getClass().getCreatureStats(player);
    MWMechanics::DynamicStat<float> health(creatureClass->getHealth());
    MWMechanics::DynamicStat<float> magicka(creatureClass->getMagicka());
    MWMechanics::DynamicStat<float> fatigue(creatureClass->getFatigue());

    static MWMechanics::DynamicStat<float> oldHealth(creatureClass->getHealth());
    static MWMechanics::DynamicStat<float> oldMagicka(creatureClass->getMagicka());
    static MWMechanics::DynamicStat<float> oldFatigue(creatureClass->getFatigue());

    static float timer = 0;
    if(((oldHealth != health || oldMagicka != magicka || oldFatigue != fatigue) &&
            (timer += MWBase::Environment::get().getFrameDuration()) >= 0.5 ) || forceUpdate)
    {
        oldHealth = health;
        oldMagicka = magicka;
        oldFatigue = fatigue;

        health.writeState(CreatureStats()->mDynamic[0]);
        magicka.writeState(CreatureStats()->mDynamic[1]);
        fatigue.writeState(CreatureStats()->mDynamic[2]);

        timer = 0;

        GetNetworking()->GetPacket(ID_GAME_UPDATE_BASESTATS)->Send(this);

    }
}

void LocalPlayer::updatePosition(bool forceUpdate)
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    MWWorld::Ptr player = world->getPlayerPtr();

    const MWMechanics::Movement &move = player.getClass().getMovementSettings(player);

    static bool posChanged = false;

    ESM::Position _pos = player.getRefData().getPosition();

    const bool isChangedPos = (move.mPosition[0] != 0 || move.mPosition[1] != 0 || move.mPosition[2] != 0
                               || move.mRotation[0] != 0 || move.mRotation[1] != 0 || move.mRotation[2] != 0);

    if (isChangedPos || posChanged || forceUpdate)
    {
        posChanged = isChangedPos;


        (*Position()) = _pos;

        Dir()->pos[0] = move.mPosition[0];
        Dir()->pos[1] = move.mPosition[1];
        Dir()->pos[2] = move.mPosition[2];

        GetNetworking()->GetPacket(ID_GAME_UPDATE_POS)->Send(this);
    }
}

void LocalPlayer::updateInventory(bool forceUpdate)
{
    MWWorld::Ptr player = GetPlayerPtr();

    static bool invChanged = false;

    if (forceUpdate)
        invChanged = true;


    MWWorld::InventoryStore &invStore = player.getClass().getInventoryStore(player);
    for (int slot = 0; slot < MWWorld::InventoryStore::Slots; ++slot)
    {
        MWWorld::ContainerStoreIterator it = invStore.getSlot(slot);
        if(it != invStore.end() && !::Misc::StringUtils::ciEqual(it->getCellRef().getRefId(), EquipedItem(slot)->refid))
        {
            invChanged = true;

            EquipedItem(slot)->refid = it->getCellRef().getRefId();
            if(slot == MWWorld::InventoryStore::Slot_CarriedRight)
            {
                MWMechanics::WeaponType weaptype;
                MWMechanics::getActiveWeapon(player.getClass().getCreatureStats(player), player.getClass().getInventoryStore(player), &weaptype);
                if(weaptype != MWMechanics::WeapType_Thrown)
                    EquipedItem(slot)->count = 1;
            }
            else
                EquipedItem(slot)->count = invStore.count(it->getCellRef().getRefId());
        }
        else if (it == invStore.end() && !EquipedItem(slot)->refid.empty())
        {
            invChanged = true;
            EquipedItem(slot)->refid = "";
            EquipedItem(slot)->count = 0;
        }
    }

    if (invChanged)
    {
        RakNet::BitStream bs;
        bs.ResetWritePointer();
        GetNetworking()->GetPacket((RakNet::MessageID) ID_GAME_UPDATE_EQUIPED)->Packet(&bs, this, true);
        GetNetworking()->SendData(&bs);
        invChanged = false;
    }
}

void LocalPlayer::updateAttackState(bool forceUpdate)
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    MWWorld::Ptr player = GetPlayerPtr();

    using namespace MWMechanics;

    static bool attackPressed = false; // prevent flood
    MWMechanics::DrawState_ state = player.getClass().getNpcStats(player).getDrawState();
    //player.getClass().hit(player, 1, ESM::Weapon::AT_Chop);
    if(world->getPlayer().getAttackingOrSpell() && !attackPressed)
    {
        MWWorld::Ptr weapon = MWWorld::Ptr(); // hand-to-hand
        //player.getClass().onHit(player, 0.5, true, weapon, 0, 1);
        if(state == MWMechanics::DrawState_Spell)
        {
            //cout << "getAttackingOrSpell" << endl;
            const string &spell = MWBase::Environment::get().getWindowManager()->getSelectedSpell();

            GetAttack()->attacker = guid;
            GetAttack()->type = 1;
            GetAttack()->pressed = true;
            GetAttack()->refid = spell;

            /*RakNet::BitStream bs;
            GetNetworking()->GetPacket((RakNet::MessageID) ID_GAME_ATTACK)->Packet(&bs, this, true);
            GetNetworking()->SendData(&bs);*/
        }
        else if(state == MWMechanics::DrawState_Weapon)
        {
            //PrepareAttack(2);
        }
        attackPressed = true;
    }
    else if(!world->getPlayer().getAttackingOrSpell() && attackPressed)
    {
        if(/*state == MWMechanics::DrawState_Spell ||*/ state == MWMechanics::DrawState_Weapon)
        {
            //localNetPlayer->GetAttack()->success = false;
            //SendAttack(0);
        }
        attackPressed = false;
    }
}

void LocalPlayer::updateDeadState(bool forceUpdate)
{
    MWWorld::Ptr player = GetPlayerPtr();

    MWMechanics::NpcStats *playerStats = &player.getClass().getNpcStats(player);
    static bool isDead = false;

    if(playerStats->isDead() && !isDead)
    {
        CreatureStats()->mDead = true;
        RakNet::BitStream bs;
        GetNetworking()->GetPacket((RakNet::MessageID)ID_GAME_DIE)->Packet(&bs, this, true);
        GetNetworking()->SendData(&bs);
        isDead = true;
    }
    else if(playerStats->getHealth().getCurrent() > 0 && isDead)
        isDead = false;
}

void LocalPlayer::updateAttributesAndSkills(bool forceUpdate)
{
    MWWorld::Ptr player = GetPlayerPtr();

    const MWMechanics::NpcStats &_npcStats = player.getClass().getNpcStats(player);

    for(int i = 0; i < PacketAttributesAndStats::StatsCount; ++i)
        _npcStats.getSkill(i).writeState( NpcStats()->mSkills[i]);

    for(int i = 0; i < PacketAttributesAndStats::AttributesCount; ++i)
        _npcStats.getAttribute(i).writeState(CreatureStats()->mAttributes[i]);
}

Networking *LocalPlayer::GetNetworking()
{
    return mwmp::Main::get().getNetworking();
}



void LocalPlayer::PrepareAttack(char type, bool state)
{
    if(GetAttack()->pressed == state)
        return;

    MWMechanics::DrawState_ dstate = GetPlayerPtr().getClass().getNpcStats(GetPlayerPtr()).getDrawState();

    if(dstate == MWMechanics::DrawState_Spell)
    {
        const string &spell = MWBase::Environment::get().getWindowManager()->getSelectedSpell();

        GetAttack()->refid = spell;
    }
    else
    {

    }

    GetAttack()->pressed = state;
    GetAttack()->type = type;
    GetAttack()->knockdown = false;
    GetAttack()->success = false;
    GetAttack()->block = false;
    GetAttack()->target = RakNet::RakNetGUID();
    GetAttack()->attacker = guid;

    RakNet::BitStream bs;
    GetNetworking()->GetPacket((RakNet::MessageID) ID_GAME_ATTACK)->Packet(&bs, this, true);
    GetNetworking()->SendData(&bs);
}


void LocalPlayer::SendAttack(char type)
{
    MWMechanics::DrawState_ state = GetPlayerPtr().getClass().getNpcStats(GetPlayerPtr()).getDrawState();

    if(state == MWMechanics::DrawState_Spell)
    {

    }
    else
    {

    }
    GetAttack()->type = type;
    GetAttack()->pressed = false;
    RakNet::BitStream bs;
    GetNetworking()->GetPacket((RakNet::MessageID) ID_GAME_ATTACK)->Packet(&bs, this, true);
    GetNetworking()->SendData(&bs);
}

void LocalPlayer::updateCell(bool forceUpdate)
{
    const ESM::Cell *_cell = MWBase::Environment::get().getWorld()->getPlayerPtr().getCell()->getCell();
    static bool isExterior = !_cell->isExterior();

    if(isExterior != _cell->isExterior() || !Misc::StringUtils::ciEqual(_cell->mName, GetCell()->mName) || forceUpdate)
    {
        (*GetCell()) = *_cell;
        isExterior = _cell->isExterior();

        RakNet::BitStream bs;
        GetNetworking()->GetPacket((RakNet::MessageID) ID_GAME_CELL)->Packet(&bs, this, true);
        GetNetworking()->SendData(&bs);
    }
}

void LocalPlayer::updateDrawStateAndFlags(bool forceUpdate)
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    MWWorld::Ptr player = world->getPlayerPtr();


    MWMechanics::NpcStats npcStats = player.getClass().getNpcStats(player);
    using namespace MWMechanics;

    static bool oldRun = npcStats.getMovementFlag(CreatureStats::Flag_Run);
    static bool oldSneak = npcStats.getMovementFlag(CreatureStats::Flag_Sneak);
    static bool oldForceJump = npcStats.getMovementFlag(CreatureStats::Flag_ForceJump);
    static bool oldForceMoveJump = npcStats.getMovementFlag(CreatureStats::Flag_ForceMoveJump);

    bool run = npcStats.getMovementFlag(CreatureStats::Flag_Run);
    bool sneak = npcStats.getMovementFlag(CreatureStats::Flag_Sneak);
    bool forceJump = npcStats.getMovementFlag(CreatureStats::Flag_ForceJump);
    bool forceMoveJump = npcStats.getMovementFlag(CreatureStats::Flag_ForceMoveJump);
    bool jump = !world->isOnGround(player) && !world->isFlying(player);
    static bool onJump = false;

    MWMechanics::DrawState_ state = player.getClass().getNpcStats(player).getDrawState();
    static MWMechanics::DrawState_ oldState = player.getClass().getNpcStats(player).getDrawState();
    //static float timer = 0;
    if(oldRun != run
          || oldSneak != sneak || oldForceJump != forceJump
          || oldForceMoveJump != forceMoveJump || oldState != state ||
            ((jump || onJump)/* && (timer += MWBase::Environment::get().getFrameDuration() )> 0.5*/)
          || forceUpdate)
    {
        oldSneak = sneak;
        oldRun = run;
        oldForceJump = forceJump;
        oldForceMoveJump = forceMoveJump;
        oldState = state;
        onJump = jump;

        movementFlags = 0;
#define __SETFLAG(flag, value) (value) ? (movementFlags | flag) : (movementFlags & ~flag)

        movementFlags = __SETFLAG(CreatureStats::Flag_Sneak, sneak);
        movementFlags = __SETFLAG(CreatureStats::Flag_Run, run);
        movementFlags = __SETFLAG(CreatureStats::Flag_ForceJump, forceJump);
        movementFlags = __SETFLAG(CreatureStats::Flag_ForceJump, jump);
        movementFlags = __SETFLAG(CreatureStats::Flag_ForceMoveJump, forceMoveJump);

#undef __SETFLAG

        if (state == MWMechanics::DrawState_Nothing)
            (*DrawState()) = 0;
        else if (state == MWMechanics::DrawState_Weapon)
            (*DrawState()) = 1;
        else if (state == MWMechanics::DrawState_Spell)
            (*DrawState()) = 2;

        if(jump)
            mwmp::Main::get().getLocalPlayer()->updatePosition(true); // fix position after jump;

        RakNet::BitStream bs;
        GetNetworking()->GetPacket((RakNet::MessageID) ID_GAME_DRAWSTATE)->Packet(&bs, this, true);
        GetNetworking()->SendData(&bs);
        //timer = 0;
    }
}

void LocalPlayer::CharGen(int stageFirst, int stageEnd)
{
    CharGenStage()->current = stageFirst;
    CharGenStage()->end = stageEnd;
}

bool LocalPlayer::CharGenThread() // ToDo: need fix
{
    MWBase::WindowManager *windowManager = MWBase::Environment::get().getWindowManager();
    if(windowManager->isGuiMode())
        return false;

    if(CharGenStage()->current >= CharGenStage()->end)
    {

        if (GetNetworking()->isConnected() && CharGenStage()->current == CharGenStage()->end &&
            CharGenStage()->end != 0)
        {
            MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
            (*Npc()) = *player.get<ESM::NPC>()->mBase;
            GetNetworking()->GetPacket(ID_GAME_BASE_INFO)->Send(this);
            if(CharGenStage()->end != 1)
            {
                updateBaseStats(true);
                updateAttributesAndSkills(true);
                GetNetworking()->GetPacket(ID_GAME_UPDATE_SKILLS)->Send(this);
                GetNetworking()->GetPacket(ID_GAME_CHARGEN)->Send(this);
            }
            CharGenStage()->end = 0;
            /*RakNet::BitStream bs;
            GetNetworking()->GetPacket(ID_GAME_BASE_INFO)->Packet(&bs, this, true);
            GetNetworking()->SendData(&bs);*/

        }
        return true;
    }

    switch (CharGenStage()->current)
    {
        case 0:
            windowManager->pushGuiMode(MWGui::GM_Name);
            break;
        case 1:
            windowManager->pushGuiMode(MWGui::GM_Race);
            break;
        case 2:
            windowManager->pushGuiMode(MWGui::GM_Class);
            break;
        case 3:
            windowManager->pushGuiMode(MWGui::GM_Birth);
            break;
        default:
            windowManager->pushGuiMode(MWGui::GM_Review);
            break;
    }
    GetNetworking()->GetPacket(ID_GAME_CHARGEN)->Send(this);
    CharGenStage()->current++;

    return false;
}

void LocalPlayer::updateChar()
{
    MWBase::Environment::get().getMechanicsManager()->setPlayerRace(
            Npc()->mRace,
            Npc()->isMale(),
            Npc()->mHead,
            Npc()->mHair
    );
    MWBase::Environment::get().getWindowManager()->getInventoryWindow()->rebuildAvatar();
}
