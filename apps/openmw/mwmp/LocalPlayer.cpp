//
// Created by koncord on 14.01.16.
//

#include "../mwworld/manualref.hpp"
#include "../mwmechanics/aitravel.hpp"
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
#include "../mwdialogue/dialoguemanagerimp.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwmechanics/spellcasting.hpp"
#include "../mwgui/inventorywindow.hpp"
#include <components/openmw-mp/Log.hpp>
#include <components/misc/rng.hpp>

#include "LocalPlayer.hpp"
#include "Main.hpp"

using namespace mwmp;
using namespace std;

LocalPlayer::LocalPlayer()
{
    CharGenStage()->current = 0;
    CharGenStage()->end = 1;
    consoleAllowed = true;
    ignorePosPacket = false;
}

LocalPlayer::~LocalPlayer()
{

}

Networking *LocalPlayer::getNetworking()
{
    return mwmp::Main::get().getNetworking();
}

MWWorld::Ptr LocalPlayer::getPlayerPtr()
{
    return MWBase::Environment::get().getWorld()->getPlayerPtr();
}

void LocalPlayer::update()
{
    updateCell();
    updatePosition();
    updateDrawStateAndFlags();
    updateAttackState();
    updateDeadState();
    updateEquipment();
    updateDynamicStats();
    updateAttributes();
    updateSkills();
    updateLevel();
}

void LocalPlayer::charGen(int stageFirst, int stageEnd)
{
    CharGenStage()->current = stageFirst;
    CharGenStage()->end = stageEnd;
}

bool LocalPlayer::charGenThread()
{
    MWBase::WindowManager *windowManager = MWBase::Environment::get().getWindowManager();
    
    // If we haven't finished CharGen and we're in a menu, it must be
    // one of the CharGen menus, so go no further until it's closed
    if (windowManager->isGuiMode() && CharGenStage()->end != 0)
        return false;

    // If the current stage of CharGen is not the last one,
    // move to the next one
    else if (CharGenStage()->current < CharGenStage()->end)
    {
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
        getNetworking()->getPlayerPacket(ID_GAME_CHARGEN)->Send(this);
        CharGenStage()->current++;

        return false;
    }
    
    // If we've reached the last stage of CharGen, send the
    // corresponding packets and mark CharGen as finished
    else if (CharGenStage()->end != 0)
    {
        MWBase::World *world = MWBase::Environment::get().getWorld();
        MWWorld::Ptr player = world->getPlayerPtr();
        (*Npc()) = *player.get<ESM::NPC>()->mBase;
        (*BirthSign()) = world->getPlayer().getBirthSign();

        LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Sending ID_GAME_BASE_INFO to server with my CharGen info");
        getNetworking()->getPlayerPacket(ID_GAME_BASE_INFO)->Send(this);

        // Send stats packets if this is the 2nd round of CharGen that
        // only happens for new characters
        if (CharGenStage()->end != 1)
        {
            updateDynamicStats(true);
            updateAttributes(true);
            updateSkills(true);
            updateLevel(true);
            sendClass();
            getNetworking()->getPlayerPacket(ID_GAME_CHARGEN)->Send(this);
        }

        // Set the last stage variable to 0 to indicate that CharGen is finished
        CharGenStage()->end = 0;
    }

    return true;
}

void LocalPlayer::updateDynamicStats(bool forceUpdate)
{
    MWWorld::Ptr player = getPlayerPtr();

    MWMechanics::CreatureStats *ptrCreatureStats = &player.getClass().getCreatureStats(player);
    MWMechanics::DynamicStat<float> health(ptrCreatureStats->getHealth());
    MWMechanics::DynamicStat<float> magicka(ptrCreatureStats->getMagicka());
    MWMechanics::DynamicStat<float> fatigue(ptrCreatureStats->getFatigue());

    static MWMechanics::DynamicStat<float> oldHealth(ptrCreatureStats->getHealth());
    static MWMechanics::DynamicStat<float> oldMagicka(ptrCreatureStats->getMagicka());
    static MWMechanics::DynamicStat<float> oldFatigue(ptrCreatureStats->getFatigue());

    static float timer = 0;

    if ((timer += MWBase::Environment::get().getFrameDuration()) >= 0.5 || forceUpdate)
    {
        if (oldHealth != health || oldMagicka != magicka || oldFatigue != fatigue || forceUpdate)
        {
            oldHealth = health;
            oldMagicka = magicka;
            oldFatigue = fatigue;

            health.writeState(CreatureStats()->mDynamic[0]);
            magicka.writeState(CreatureStats()->mDynamic[1]);
            fatigue.writeState(CreatureStats()->mDynamic[2]);

            timer = 0;

            getNetworking()->getPlayerPacket(ID_GAME_DYNAMICSTATS)->Send(this);
        }
    }
}

void LocalPlayer::updateAttributes(bool forceUpdate)
{
    MWWorld::Ptr player = getPlayerPtr();
    const MWMechanics::NpcStats &ptrNpcStats = player.getClass().getNpcStats(player);
    bool attributesChanged = false;

    for (int i = 0; i < 8; ++i)
    {
        if (ptrNpcStats.getAttribute(i).getBase() != CreatureStats()->mAttributes[i].mBase)
        {
            ptrNpcStats.getAttribute(i).writeState(CreatureStats()->mAttributes[i]);
            attributesChanged = true;
        }
    }

    if (attributesChanged || forceUpdate)
    {
        getNetworking()->getPlayerPacket(ID_GAME_ATTRIBUTE)->Send(this);
    }
}

void LocalPlayer::updateSkills(bool forceUpdate)
{
    MWWorld::Ptr player = getPlayerPtr();
    const MWMechanics::NpcStats &ptrNpcStats = player.getClass().getNpcStats(player);

    // Track whether skills have changed their values, but not whether
    // progress towards skill increases has changed (to not spam server
    // with packets every time tiny progress is made)
    bool skillsChanged = false;

    for (int i = 0; i < 27; ++i)
    {
        if (ptrNpcStats.getSkill(i).getBase() != NpcStats()->mSkills[i].mBase)
        {
            ptrNpcStats.getSkill(i).writeState(NpcStats()->mSkills[i]);
            skillsChanged = true;
        }
        // If we only have skill progress, remember it for future packets,
        // but don't send a packet just because of this
        else if (ptrNpcStats.getSkill(i).getProgress() != NpcStats()->mSkills[i].mProgress)
        {
            ptrNpcStats.getSkill(i).writeState(NpcStats()->mSkills[i]);
        }    
    }

    for (int i = 0; i < 8; i++)
    {
        if (ptrNpcStats.getSkillIncrease(i) != NpcStats()->mSkillIncrease[i]) {
            NpcStats()->mSkillIncrease[i] = ptrNpcStats.getSkillIncrease(i);
        }
    }

    if (skillsChanged || forceUpdate)
    {
        NpcStats()->mLevelProgress = ptrNpcStats.getLevelProgress();
        getNetworking()->getPlayerPacket(ID_GAME_SKILL)->Send(this);
    }
}

void LocalPlayer::updateLevel(bool forceUpdate)
{
    MWWorld::Ptr player = getPlayerPtr();
    const MWMechanics::NpcStats &ptrNpcStats = player.getClass().getNpcStats(player);

    if (ptrNpcStats.getLevel() != CreatureStats()->mLevel || forceUpdate)
    {
        CreatureStats()->mLevel = ptrNpcStats.getLevel();
        getNetworking()->getPlayerPacket(ID_GAME_LEVEL)->Send(this);

        // Also update skills to refresh level progress and attribute bonuses
        // for next level up
        updateSkills(true);
    }
}

void LocalPlayer::updatePosition(bool forceUpdate)
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    MWWorld::Ptr player = world->getPlayerPtr();

    const MWMechanics::Movement &move = player.getClass().getMovementSettings(player);

    static bool posChanged = false;

    static bool isJumping = false;
    static bool sentJumpEnd = true;

    ESM::Position ptrPos = player.getRefData().getPosition();

    const bool isChangedPos = (move.mPosition[0] != 0 || move.mPosition[1] != 0 || move.mPosition[2] != 0
                               || move.mRotation[0] != 0 || move.mRotation[1] != 0 || move.mRotation[2] != 0);

    if (isChangedPos || posChanged || forceUpdate)
    {
        posChanged = isChangedPos;

        if (!isJumping && !world->isOnGround(player) && !world->isFlying(player))
        {
            isJumping = true;
        }

        (*Position()) = ptrPos;

        Dir()->pos[0] = move.mPosition[0];
        Dir()->pos[1] = move.mPosition[1];
        Dir()->pos[2] = move.mPosition[2];

        getNetworking()->getPlayerPacket(ID_GAME_POS)->Send(this);
    }
    else if (isJumping && world->isOnGround(player))
    {
        isJumping = false;
        sentJumpEnd = false;
    }
    // Packet with jump end position has to be sent one tick after above check
    else if (!sentJumpEnd)
    {
        sentJumpEnd = true;
        (*Position()) = ptrPos;
        getNetworking()->getPlayerPacket(ID_GAME_POS)->Send(this);
    }
}

void LocalPlayer::updateCell(bool forceUpdate)
{
    const ESM::Cell *ptrCell = MWBase::Environment::get().getWorld()->getPlayerPtr().getCell()->getCell();
    bool cellChanged = false;

    // Send a packet to server to update this LocalPlayer's cell if:
    // 1) forceUpdate is true
    // 2) The LocalPlayer's cell name does not equal the World Player's cell name
    // 3) The LocalPlayer's exterior cell coordinates do not equal the World Player's
    //    exterior cell coordinates
    if (forceUpdate)
    {
        cellChanged = true;
    }
    else if (!Misc::StringUtils::ciEqual(ptrCell->mName, getCell()->mName))
    {
        cellChanged = true;
    }
    else if (ptrCell->isExterior())
    {
        if (ptrCell->mData.mX != getCell()->mData.mX)
        {
            cellChanged = true;
        }
        else if (ptrCell->mData.mY != getCell()->mData.mY)
        {
            cellChanged = true;
        }
    }

    if (cellChanged)
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Sending ID_GAME_CELL to server");

        LOG_APPEND(Log::LOG_INFO, "- Moved from %s to %s",
            getCell()->getDescription().c_str(),
            ptrCell->getDescription().c_str());

        (*getCell()) = *ptrCell;

        // Make sure the position is updated before a cell packet is sent, or else
        // cell change events in server scripts will have the wrong player position
        updatePosition(true);

        RakNet::BitStream bs;
        getNetworking()->getPlayerPacket((RakNet::MessageID) ID_GAME_CELL)->Packet(&bs, this, true);
        getNetworking()->sendData(&bs);

        // Also force an update to skills (to send all progress to skill increases)
        updateSkills(true);

        // Also check if the inventory needs to be updated
        updateInventory();
    }
}

void LocalPlayer::updateChar()
{
    MWBase::Environment::get().getMechanicsManager()->setPlayerRace(
        Npc()->mRace,
        Npc()->isMale(),
        Npc()->mHead,
        Npc()->mHair
    );

    MWBase::Environment::get().getMechanicsManager()->setPlayerBirthsign(*BirthSign());

    MWBase::Environment::get().getWindowManager()->getInventoryWindow()->rebuildAvatar();
}

void LocalPlayer::updateEquipment(bool forceUpdate)
{
    MWWorld::Ptr player = getPlayerPtr();

    static bool equipChanged = false;

    if (forceUpdate)
        equipChanged = true;

    MWWorld::InventoryStore &invStore = player.getClass().getInventoryStore(player);
    for (int slot = 0; slot < MWWorld::InventoryStore::Slots; slot++)
    {
        MWWorld::ContainerStoreIterator it = invStore.getSlot(slot);
        if (it != invStore.end() && !::Misc::StringUtils::ciEqual(it->getCellRef().getRefId(), EquipedItem(slot)->refid))
        {
            equipChanged = true;

            EquipedItem(slot)->refid = it->getCellRef().getRefId();
            EquipedItem(slot)->health = it->getCellRef().getCharge();
            if (slot == MWWorld::InventoryStore::Slot_CarriedRight)
            {
                MWMechanics::WeaponType weaptype;
                MWMechanics::getActiveWeapon(player.getClass().getCreatureStats(player), player.getClass().getInventoryStore(player), &weaptype);
                if (weaptype != MWMechanics::WeapType_Thrown)
                    EquipedItem(slot)->count = 1;
            }
            else
                EquipedItem(slot)->count = invStore.count(it->getCellRef().getRefId());
        }
        else if (it == invStore.end() && !EquipedItem(slot)->refid.empty())
        {
            equipChanged = true;
            EquipedItem(slot)->refid = "";
            EquipedItem(slot)->count = 0;
            EquipedItem(slot)->health = 0;
        }
    }

    if (equipChanged)
    {
        RakNet::BitStream bs;
        bs.ResetWritePointer();
        getNetworking()->getPlayerPacket((RakNet::MessageID) ID_GAME_EQUIPMENT)->Packet(&bs, this, true);
        getNetworking()->sendData(&bs);
        equipChanged = false;
    }
}

void LocalPlayer::updateInventory(bool forceUpdate)
{
    static bool invChanged = false;
    
    if (forceUpdate)
        invChanged = true;

    MWWorld::Ptr ptrPlayer = getPlayerPtr();
    MWWorld::InventoryStore &ptrInventory = ptrPlayer.getClass().getInventoryStore(ptrPlayer);
    mwmp::Item item;

    if (!invChanged)
    {
        for (vector<Item>::iterator iter = inventory.items.begin(); iter != inventory.items.end(); ++iter)
        {
            MWWorld::ContainerStoreIterator result(ptrInventory.begin());
            for (; result != ptrInventory.end(); ++result)
            {
                item.refid = result->getCellRef().getRefId();
                if (item.refid.find("$dynamic") != string::npos) // skip generated items (self enchanted for e.g.)
                    continue;

                item.count = result->getRefData().getCount();
                item.health = result->getCellRef().getCharge();

                if (item == (*iter))
                    break;
            }
            if (result == ptrInventory.end())
            {
                invChanged = true;
                break;
            }
        }
    }
    
    if (!invChanged)
    {
        for (MWWorld::ContainerStoreIterator iter(ptrInventory.begin()); iter != ptrInventory.end(); ++iter)
        {
            item.refid = iter->getCellRef().getRefId();
            if (item.refid.find("$dynamic") != string::npos) // skip generated items (self enchanted for e.g.)
                continue;

            item.count = iter->getRefData().getCount();
            item.health = iter->getCellRef().getCharge();

            vector<Item>::iterator result = inventory.items.begin();

            for (; result != inventory.items.end(); result++)
            {
                if ((*result) == item)
                    break;
            }

            if (result == inventory.items.end())
            {
                invChanged = true;
                break;
            }
        }
    }

    if (!invChanged)
        return;

    invChanged = false;

    sendInventory();
}

void LocalPlayer::updateAttackState(bool forceUpdate)
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    MWWorld::Ptr player = getPlayerPtr();

    using namespace MWMechanics;

    static bool attackPressed = false; // prevent flood
    MWMechanics::DrawState_ state = player.getClass().getNpcStats(player).getDrawState();
    //player.getClass().hit(player, 1, ESM::Weapon::AT_Chop);
    if (world->getPlayer().getAttackingOrSpell() && !attackPressed)
    {
        MWWorld::Ptr weapon = MWWorld::Ptr(); // hand-to-hand
                                              //player.getClass().onHit(player, 0.5, true, weapon, 0, 1);
        if (state == MWMechanics::DrawState_Spell)
        {
            const string &spell = MWBase::Environment::get().getWindowManager()->getSelectedSpell();

            getAttack()->attacker = guid;
            getAttack()->type = Attack::MAGIC;
            getAttack()->pressed = true;
            getAttack()->refid = spell;

            /*RakNet::BitStream bs;
            getNetworking()->getPlayerPacket((RakNet::MessageID) ID_GAME_ATTACK)->Packet(&bs, this, true);
            getNetworking()->SendData(&bs);*/
        }
        else if (state == MWMechanics::DrawState_Weapon)
        {
            //PrepareAttack(2);
        }
        attackPressed = true;
    }
    else if (!world->getPlayer().getAttackingOrSpell() && attackPressed)
    {
        if (/*state == MWMechanics::DrawState_Spell ||*/ state == MWMechanics::DrawState_Weapon)
        {
            //localNetPlayer->getAttack()->success = false;
            //SendAttack(0);
        }
        attackPressed = false;
    }
}

void LocalPlayer::updateDeadState(bool forceUpdate)
{
    MWWorld::Ptr player = getPlayerPtr();

    MWMechanics::NpcStats *ptrNpcStats = &player.getClass().getNpcStats(player);
    static bool isDead = false;

    if (ptrNpcStats->isDead() && !isDead)
    {
        CreatureStats()->mDead = true;
        RakNet::BitStream bs;
        getNetworking()->getPlayerPacket((RakNet::MessageID)ID_GAME_DIE)->Packet(&bs, this, true);
        getNetworking()->sendData(&bs);
        isDead = true;
    }
    else if (ptrNpcStats->getHealth().getCurrent() > 0 && isDead)
        isDead = false;
}

void LocalPlayer::updateDrawStateAndFlags(bool forceUpdate)
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    MWWorld::Ptr player = world->getPlayerPtr();


    MWMechanics::NpcStats ptrNpcStats = player.getClass().getNpcStats(player);
    using namespace MWMechanics;

    static bool oldRun = ptrNpcStats.getMovementFlag(CreatureStats::Flag_Run);
    static bool oldSneak = ptrNpcStats.getMovementFlag(CreatureStats::Flag_Sneak);
    static bool oldForceJump = ptrNpcStats.getMovementFlag(CreatureStats::Flag_ForceJump);
    static bool oldForceMoveJump = ptrNpcStats.getMovementFlag(CreatureStats::Flag_ForceMoveJump);

    bool run = ptrNpcStats.getMovementFlag(CreatureStats::Flag_Run);
    bool sneak = ptrNpcStats.getMovementFlag(CreatureStats::Flag_Sneak);
    bool forceJump = ptrNpcStats.getMovementFlag(CreatureStats::Flag_ForceJump);
    bool forceMoveJump = ptrNpcStats.getMovementFlag(CreatureStats::Flag_ForceMoveJump);
    bool jump = !world->isOnGround(player) && !world->isFlying(player);
    static bool onJump = false;

    MWMechanics::DrawState_ state = player.getClass().getNpcStats(player).getDrawState();
    static MWMechanics::DrawState_ oldState = player.getClass().getNpcStats(player).getDrawState();
    //static float timer = 0;
    if (oldRun != run
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

        if (jump)
            mwmp::Main::get().getLocalPlayer()->updatePosition(true); // fix position after jump;

        RakNet::BitStream bs;
        getNetworking()->getPlayerPacket((RakNet::MessageID) ID_GAME_DRAWSTATE)->Packet(&bs, this, true);
        getNetworking()->sendData(&bs);
        //timer = 0;
    }
}

void LocalPlayer::setDynamicStats()
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    MWWorld::Ptr player = world->getPlayerPtr();

    MWMechanics::CreatureStats *ptrCreatureStats = &player.getClass().getCreatureStats(player);
    MWMechanics::DynamicStat<float> dynamicStat;

    for (int i = 0; i < 3; ++i)
    {
        dynamicStat = ptrCreatureStats->getDynamic(i);
        dynamicStat.setBase(CreatureStats()->mDynamic[i].mBase);
        dynamicStat.setCurrent(CreatureStats()->mDynamic[i].mCurrent);
        ptrCreatureStats->setDynamic(i, dynamicStat);
    }
}

void LocalPlayer::setAttributes()
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    MWWorld::Ptr player = world->getPlayerPtr();

    MWMechanics::CreatureStats *ptrCreatureStats = &player.getClass().getCreatureStats(player);
    MWMechanics::AttributeValue attributeValue;

    for (int i = 0; i < 8; ++i)
    {
        attributeValue.readState(CreatureStats()->mAttributes[i]);
        ptrCreatureStats->setAttribute(i, attributeValue);
    }
}

void LocalPlayer::setSkills()
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    MWWorld::Ptr player = world->getPlayerPtr();

    MWMechanics::NpcStats *ptrNpcStats = &player.getClass().getNpcStats(player);
    MWMechanics::SkillValue skillValue;

    for (int i = 0; i < 27; ++i)
    {
        skillValue.readState(NpcStats()->mSkills[i]);
        ptrNpcStats->setSkill(i, skillValue);
    }

    for (int i = 0; i < 8; ++i)
    {
        ptrNpcStats->setSkillIncrease(i, NpcStats()->mSkillIncrease[i]);
    }

    ptrNpcStats->setLevelProgress(NpcStats()->mLevelProgress);
}

void LocalPlayer::setLevel()
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    MWWorld::Ptr player = world->getPlayerPtr();

    MWMechanics::CreatureStats *ptrCreatureStats = &player.getClass().getCreatureStats(player);
    ptrCreatureStats->setLevel(CreatureStats()->mLevel);
}

void LocalPlayer::setPosition()
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    MWWorld::Ptr player = world->getPlayerPtr();

    // If we're ignoring this position packet because of an invalid cell change,
    // don't make the next one get ignored as well
    if (ignorePosPacket)
    {
        ignorePosPacket = false;
    }
    else
    {
        world->getPlayer().setTeleported(true);
        world->moveObject(player, Position()->pos[0], Position()->pos[1], Position()->pos[2]);
        world->rotateObject(player, Position()->rot[0], Position()->rot[1], Position()->rot[2]);
    }

    updatePosition(true);
}

void LocalPlayer::setCell()
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    MWWorld::Ptr player = world->getPlayerPtr();
    ESM::Position pos;

    world->getPlayer().setTeleported(true);

    int x = getCell()->mData.mX;
    int y = getCell()->mData.mY;

    if (getCell()->isExterior())
    {
        world->indexToPosition(x, y, pos.pos[0], pos.pos[1], true);
        pos.pos[2] = 0;

        pos.rot[0] = pos.rot[1] = pos.rot[2] = 0;

        world->changeToExteriorCell(pos, true);
        world->fixPosition(player);
    }
    else if (world->findExteriorPosition(getCell()->mName, pos))
    {
        world->changeToExteriorCell(pos, true);
        world->fixPosition(player);
    }
    else
    {
        try
        {
            world->findInteriorPosition(getCell()->mName, pos);
            world->changeToInteriorCell(getCell()->mName, pos, true);
        }
        // If we've been sent to an invalid interior, ignore the incoming
        // packet about our position in that cell
        catch (std::exception&)
        {
            LOG_APPEND(Log::LOG_INFO, "%s", "- Cell doesn't exist on this client");
            ignorePosPacket = true;
        }
    }

    updateCell(true);
}

void LocalPlayer::setClass()
{
    if (charClass.mId.empty()) // custom class
    {
        charClass.mData.mIsPlayable = 0x1;
        MWBase::Environment::get().getMechanicsManager()->setPlayerClass(charClass);
        MWBase::Environment::get().getWindowManager()->setPlayerClass(charClass);
    }
    else
    {
        MWBase::Environment::get().getMechanicsManager()->setPlayerClass(charClass.mId);

        const ESM::Class *existingCharClass = MWBase::Environment::get().getWorld()->getStore().get<ESM::Class>().find(charClass.mId);

        if (existingCharClass)
            MWBase::Environment::get().getWindowManager()->setPlayerClass(charClass);
    }
}

void LocalPlayer::setEquipment()
{
    MWWorld::Ptr ptrPlayer = getPlayerPtr();

    MWWorld::InventoryStore &ptrInventory = ptrPlayer.getClass().getInventoryStore(ptrPlayer);

    for (int slot = 0; slot < MWWorld::InventoryStore::Slots; slot++)
    {
        mwmp::Item *currentItem = EquipedItem(slot);

        if (!currentItem->refid.empty())
        {
            MWWorld::ContainerStoreIterator it = ptrInventory.begin();
            for (; it != ptrInventory.end(); ++it) // find item in inventory
            {
                if (::Misc::StringUtils::ciEqual(it->getCellRef().getRefId(), currentItem->refid))
                    break;
            }
            if (it == ptrInventory.end()) // if not exists add item
                ptrInventory.equip(slot, ptrInventory.ContainerStore::add(EquipedItem(slot)->refid.c_str(), 1, ptrPlayer), ptrPlayer);
            else
                ptrInventory.equip(slot, it, ptrPlayer);
        }
    }
}

void LocalPlayer::sendClass()
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    const ESM::NPC *cpl = world->getPlayerPtr().get<ESM::NPC>()->mBase;
    const ESM::Class *cls = world->getStore().get<ESM::Class>().find(cpl->mClass);

    if (cpl->mClass.find("$dynamic") != string::npos) // custom class
    {
        charClass.mId = "";
        charClass.mName = cls->mName;
        charClass.mDescription = cls->mDescription;
        charClass.mData = cls->mData;
    }
    else
        charClass.mId = cls->mId;

    getNetworking()->getPlayerPacket(ID_GAME_CHARCLASS)->Send(this);
}

void LocalPlayer::sendInventory()
{
    MWWorld::Ptr ptrPlayer = getPlayerPtr();
    MWWorld::InventoryStore &ptrInventory = ptrPlayer.getClass().getInventoryStore(ptrPlayer);
    mwmp::Item item;

    inventory.items.clear();

    for (MWWorld::ContainerStoreIterator iter(ptrInventory.begin()); iter != ptrInventory.end(); ++iter)
    {
        item.refid = iter->getCellRef().getRefId();
        if (item.refid.find("$dynamic") != string::npos) // skip generated items (self enchanted for e.g.)
            continue;

        item.count = iter->getRefData().getCount();
        item.health = iter->getCellRef().getCharge();

        inventory.items.push_back(item);
    }

    inventory.count = (unsigned int)inventory.items.size();
    inventory.action = Inventory::UPDATE;
    Main::get().getNetworking()->getPlayerPacket(ID_GAME_INVENTORY)->Send(this);
}

void LocalPlayer::sendAttack(Attack::TYPE type)
{
    MWMechanics::DrawState_ state = getPlayerPtr().getClass().getNpcStats(getPlayerPtr()).getDrawState();


    getAttack()->type = type;
    getAttack()->pressed = false;
    RakNet::BitStream bs;
    getNetworking()->getPlayerPacket((RakNet::MessageID) ID_GAME_ATTACK)->Packet(&bs, this, true);
    getNetworking()->sendData(&bs);
}

void LocalPlayer::prepareAttack(Attack::TYPE type, bool state)
{
    if (getAttack()->pressed == state && type != Attack::MAGIC)
        return;

    MWMechanics::DrawState_ dstate = getPlayerPtr().getClass().getNpcStats(getPlayerPtr()).getDrawState();

    if (dstate == MWMechanics::DrawState_Spell)
    {
        const string &spell = MWBase::Environment::get().getWindowManager()->getSelectedSpell();
        getAttack()->success = Misc::Rng::roll0to99() < MWMechanics::getSpellSuccessChance(spell, getPlayerPtr());
        state = true;
        getAttack()->refid = spell;
    }
    else
    {
        getAttack()->success = false;
    }

    getAttack()->pressed = state;
    getAttack()->type = type;
    getAttack()->knockdown = false;
    getAttack()->block = false;
    getAttack()->target = RakNet::RakNetGUID();
    getAttack()->attacker = guid;

    RakNet::BitStream bs;
    getNetworking()->getPlayerPacket((RakNet::MessageID) ID_GAME_ATTACK)->Packet(&bs, this, true);
    getNetworking()->sendData(&bs);
}
