//
// Created by koncord on 14.01.16.
//

#include <components/esm/esmwriter.hpp>
#include <components/openmw-mp/Log.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/journal.hpp"

#include "../mwclass/creature.hpp"
#include "../mwclass/npc.hpp"

#include "../mwdialogue/dialoguemanagerimp.hpp"

#include "../mwgui/inventorywindow.hpp"
#include "../mwgui/windowmanagerimp.hpp"

#include "../mwinput/inputmanagerimp.hpp"

#include "../mwmechanics/aitravel.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/mechanicsmanagerimp.hpp"

#include "../mwscript/scriptmanagerimp.hpp"

#include "../mwstate/statemanagerimp.hpp"

#include "../mwworld/cellstore.hpp"
#include "../mwworld/customdata.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/manualref.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/worldimp.hpp"

#include "LocalPlayer.hpp"
#include "Main.hpp"
#include "Networking.hpp"
#include "CellController.hpp"
#include "MechanicsHelper.hpp"

using namespace mwmp;
using namespace std;

LocalPlayer::LocalPlayer()
{
    charGenStage.current = 0;
    charGenStage.end = 1;

    consoleAllowed = false;
    difficulty = 0;

    ignorePosPacket = false;
    ignoreJailTeleportation = false;
    ignoreJailSkillIncreases = false;
    
    attack.shouldSend = false;

    deathReason = "suicide";
    isChangingRegion = false;

    jailProgressText = "";
    jailEndText = "";

    isWerewolf = false;

    diedSinceArrestAttempt = false;
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
    static float updateTimer = 0;
    const float timeoutSec = 0.015;

    if ((updateTimer += MWBase::Environment::get().getFrameDuration()) >= timeoutSec)
    {
        updateTimer = 0;
        updateCell();
        updatePosition();
        updateAnimFlags();
        updateAttack();
        updateDeadState();
        updateEquipment();
        updateStatsDynamic();
        updateAttributes();
        updateSkills();
        updateLevel();
        updateBounty();
    }
}

void LocalPlayer::charGen(int stageFirst, int stageEnd)
{
    charGenStage.current = stageFirst;
    charGenStage.end = stageEnd;
}

bool LocalPlayer::charGenThread()
{
    MWBase::WindowManager *windowManager = MWBase::Environment::get().getWindowManager();

    // If we haven't finished CharGen and we're in a menu, it must be
    // one of the CharGen menus, so go no further until it's closed
    if (windowManager->isGuiMode() && charGenStage.end != 0)
        return false;

    // If the current stage of CharGen is not the last one,
    // move to the next one
    else if (charGenStage.current < charGenStage.end)
    {
        switch (charGenStage.current)
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
        getNetworking()->getPlayerPacket(ID_PLAYER_CHARGEN)->setPlayer(this);
        getNetworking()->getPlayerPacket(ID_PLAYER_CHARGEN)->Send();
        charGenStage.current++;

        return false;
    }

    // If we've reached the last stage of CharGen, send the
    // corresponding packets and mark CharGen as finished
    else if (charGenStage.end != 0)
    {
        MWBase::World *world = MWBase::Environment::get().getWorld();
        MWWorld::Ptr ptrPlayer = world->getPlayerPtr();
        npc = *ptrPlayer.get<ESM::NPC>()->mBase;
        birthsign = world->getPlayer().getBirthSign();

        LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Sending ID_PLAYER_BASEINFO to server with my CharGen info");
        getNetworking()->getPlayerPacket(ID_PLAYER_BASEINFO)->setPlayer(this);
        getNetworking()->getPlayerPacket(ID_PLAYER_BASEINFO)->Send();

        // Send stats packets if this is the 2nd round of CharGen that
        // only happens for new characters
        if (charGenStage.end != 1)
        {
            updateStatsDynamic(true);
            updateAttributes(true);
            updateSkills(true);
            updateLevel(true);
            sendClass();
            sendSpellbook();
            getNetworking()->getPlayerPacket(ID_PLAYER_CHARGEN)->setPlayer(this);
            getNetworking()->getPlayerPacket(ID_PLAYER_CHARGEN)->Send();
        }

        sendCellStates();

        // Set the last stage variable to 0 to indicate that CharGen is finished
        charGenStage.end = 0;
    }

    return true;
}

bool LocalPlayer::hasFinishedCharGen()
{
    return charGenStage.end == 0;
}

void LocalPlayer::updateStatsDynamic(bool forceUpdate)
{
    MWWorld::Ptr ptrPlayer = getPlayerPtr();

    MWMechanics::CreatureStats *ptrCreatureStats = &ptrPlayer.getClass().getCreatureStats(ptrPlayer);
    MWMechanics::DynamicStat<float> health(ptrCreatureStats->getHealth());
    MWMechanics::DynamicStat<float> magicka(ptrCreatureStats->getMagicka());
    MWMechanics::DynamicStat<float> fatigue(ptrCreatureStats->getFatigue());

    static MWMechanics::DynamicStat<float> oldHealth(ptrCreatureStats->getHealth());
    static MWMechanics::DynamicStat<float> oldMagicka(ptrCreatureStats->getMagicka());
    static MWMechanics::DynamicStat<float> oldFatigue(ptrCreatureStats->getFatigue());


    // Update stats when they become 0 or they have changed enough
    auto needUpdate = [](MWMechanics::DynamicStat<float> &oldVal, MWMechanics::DynamicStat<float> &newVal, int limit) {
        return oldVal != newVal && (newVal.getCurrent() == 0 || oldVal.getCurrent() == 0
                                    || abs(oldVal.getCurrent() - newVal.getCurrent()) >= limit);
    };

    if (forceUpdate || needUpdate(oldHealth, health, 3) || needUpdate(oldMagicka, magicka, 7) ||
        needUpdate(oldFatigue, fatigue, 7))
    {
        oldHealth = health;
        oldMagicka = magicka;
        oldFatigue = fatigue;

        health.writeState(creatureStats.mDynamic[0]);
        magicka.writeState(creatureStats.mDynamic[1]);
        fatigue.writeState(creatureStats.mDynamic[2]);

        getNetworking()->getPlayerPacket(ID_PLAYER_STATS_DYNAMIC)->setPlayer(this);
        getNetworking()->getPlayerPacket(ID_PLAYER_STATS_DYNAMIC)->Send();
    }
}

void LocalPlayer::updateAttributes(bool forceUpdate)
{
    // Only send attributes if we are not a werewolf, or they will be
    // overwritten by the werewolf ones
    if (isWerewolf) return;

    MWWorld::Ptr ptrPlayer = getPlayerPtr();
    const MWMechanics::NpcStats &ptrNpcStats = ptrPlayer.getClass().getNpcStats(ptrPlayer);
    bool attributesChanged = false;

    for (int i = 0; i < 8; ++i)
    {
        if (ptrNpcStats.getAttribute(i).getBase() != creatureStats.mAttributes[i].mBase)
        {
            ptrNpcStats.getAttribute(i).writeState(creatureStats.mAttributes[i]);
            attributesChanged = true;
        }
    }

    if (attributesChanged || forceUpdate)
    {
        getNetworking()->getPlayerPacket(ID_PLAYER_ATTRIBUTE)->setPlayer(this);
        getNetworking()->getPlayerPacket(ID_PLAYER_ATTRIBUTE)->Send();
    }
}

void LocalPlayer::updateSkills(bool forceUpdate)
{
    // Only send skills if we are not a werewolf, or they will be
    // overwritten by the werewolf ones
    if (isWerewolf) return;

    MWWorld::Ptr ptrPlayer = getPlayerPtr();
    const MWMechanics::NpcStats &ptrNpcStats = ptrPlayer.getClass().getNpcStats(ptrPlayer);

    // Track whether skills have changed their values, but not whether
    // progress towards skill increases has changed (to not spam server
    // with packets every time tiny progress is made)
    bool skillsChanged = false;

    for (int i = 0; i < 27; ++i)
    {
        if (ptrNpcStats.getSkill(i).getBase() != npcStats.mSkills[i].mBase)
        {
            ptrNpcStats.getSkill(i).writeState(npcStats.mSkills[i]);
            skillsChanged = true;
        }
        // If we only have skill progress, remember it for future packets,
        // but don't send a packet just because of this
        else if (ptrNpcStats.getSkill(i).getProgress() != npcStats.mSkills[i].mProgress)
            ptrNpcStats.getSkill(i).writeState(npcStats.mSkills[i]);
    }

    for (int i = 0; i < 8; i++)
    {
        if (ptrNpcStats.getSkillIncrease(i) != npcStats.mSkillIncrease[i])
            npcStats.mSkillIncrease[i] = ptrNpcStats.getSkillIncrease(i);
    }

    if (skillsChanged || forceUpdate)
    {
        npcStats.mLevelProgress = ptrNpcStats.getLevelProgress();
        getNetworking()->getPlayerPacket(ID_PLAYER_SKILL)->setPlayer(this);
        getNetworking()->getPlayerPacket(ID_PLAYER_SKILL)->Send();
    }
}

void LocalPlayer::updateLevel(bool forceUpdate)
{
    MWWorld::Ptr ptrPlayer = getPlayerPtr();
    const MWMechanics::CreatureStats &ptrCreatureStats = ptrPlayer.getClass().getCreatureStats(ptrPlayer);

    if (ptrCreatureStats.getLevel() != creatureStats.mLevel || forceUpdate)
    {
        creatureStats.mLevel = ptrCreatureStats.getLevel();
        getNetworking()->getPlayerPacket(ID_PLAYER_LEVEL)->setPlayer(this);
        getNetworking()->getPlayerPacket(ID_PLAYER_LEVEL)->Send();

        // Also update skills to refresh level progress and attribute bonuses
        // for next level up
        updateSkills(true);
    }
}

void LocalPlayer::updateBounty(bool forceUpdate)
{
    MWWorld::Ptr ptrPlayer = getPlayerPtr();
    const MWMechanics::NpcStats &ptrNpcStats = ptrPlayer.getClass().getNpcStats(ptrPlayer);

    if (ptrNpcStats.getBounty() != npcStats.mBounty || forceUpdate)
    {
        npcStats.mBounty = ptrNpcStats.getBounty();
        getNetworking()->getPlayerPacket(ID_PLAYER_BOUNTY)->setPlayer(this);
        getNetworking()->getPlayerPacket(ID_PLAYER_BOUNTY)->Send();
    }
}

void LocalPlayer::updatePosition(bool forceUpdate)
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    MWWorld::Ptr ptrPlayer = world->getPlayerPtr();

    static bool posWasChanged = false;
    static bool isJumping = false;
    static bool sentJumpEnd = true;
    static float oldRot[2] = {0};

    position = ptrPlayer.getRefData().getPosition();

    bool posIsChanging = (direction.pos[0] != 0 || direction.pos[1] != 0 ||
            position.rot[0] != oldRot[0] || position.rot[2] != oldRot[1]);

    if (forceUpdate || posIsChanging || posWasChanged)
    {
        oldRot[0] = position.rot[0];
        oldRot[1] = position.rot[2];

        posWasChanged = posIsChanging;

        if (!isJumping && !world->isOnGround(ptrPlayer) && !world->isFlying(ptrPlayer))
            isJumping = true;

        getNetworking()->getPlayerPacket(ID_PLAYER_POSITION)->setPlayer(this);
        getNetworking()->getPlayerPacket(ID_PLAYER_POSITION)->Send();
    }
    else if (isJumping && world->isOnGround(ptrPlayer))
    {
        isJumping = false;
        sentJumpEnd = false;
    }
    // Packet with jump end position has to be sent one tick after above check
    else if (!sentJumpEnd)
    {
        sentJumpEnd = true;
        position = ptrPlayer.getRefData().getPosition();
        getNetworking()->getPlayerPacket(ID_PLAYER_POSITION)->setPlayer(this);
        getNetworking()->getPlayerPacket(ID_PLAYER_POSITION)->Send();
    }
}

void LocalPlayer::updateCell(bool forceUpdate)
{
    const ESM::Cell *ptrCell = MWBase::Environment::get().getWorld()->getPlayerPtr().getCell()->getCell();

    // If the LocalPlayer's Ptr cell is different from the LocalPlayer's packet cell, proceed
    if (forceUpdate || !Main::get().getCellController()->isSameCell(*ptrCell, cell))
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Sending ID_PLAYER_CELL_CHANGE about LocalPlayer to server");

        LOG_APPEND(Log::LOG_INFO, "- Moved from %s to %s", cell.getDescription().c_str(),
                   ptrCell->getDescription().c_str());

        if (!Misc::StringUtils::ciEqual(cell.mRegion, ptrCell->mRegion))
        {
            LOG_APPEND(Log::LOG_INFO, "- Changed region from %s to %s",
                cell.mRegion.empty() ? "none" : cell.mRegion.c_str(),
                ptrCell->mRegion.empty() ? "none" : ptrCell->mRegion.c_str());

            isChangingRegion = true;
        }

        cell = *ptrCell;
        previousCellPosition = position;

        // Make sure the position is updated before a cell packet is sent, or else
        // cell change events in server scripts will have the wrong player position
        updatePosition(true);

        getNetworking()->getPlayerPacket(ID_PLAYER_CELL_CHANGE)->setPlayer(this);
        getNetworking()->getPlayerPacket(ID_PLAYER_CELL_CHANGE)->Send();

        isChangingRegion = false;

        // Also force an update to skills (to send all progress to skill increases)
        updateSkills(true);

        // Also check if the inventory needs to be updated
        updateInventory();
    }
}

void LocalPlayer::updateChar()
{
    MWBase::Environment::get().getMechanicsManager()->setPlayerRace(npc.mRace, npc.isMale(), npc.mHead, npc.mHair);
    MWBase::Environment::get().getMechanicsManager()->setPlayerBirthsign(birthsign);
    MWBase::Environment::get().getWindowManager()->getInventoryWindow()->rebuildAvatar();
}

void LocalPlayer::updateEquipment(bool forceUpdate)
{
    MWWorld::Ptr ptrPlayer = getPlayerPtr();

    static bool equipmentChanged = false;

    if (forceUpdate)
        equipmentChanged = true;

    MWWorld::InventoryStore &invStore = ptrPlayer.getClass().getInventoryStore(ptrPlayer);
    for (int slot = 0; slot < MWWorld::InventoryStore::Slots; slot++)
    {
        auto &item = equipedItems[slot];
        MWWorld::ContainerStoreIterator it = invStore.getSlot(slot);
        if (it != invStore.end())
        {
            if (!::Misc::StringUtils::ciEqual(it->getCellRef().getRefId(), equipedItems[slot].refId))
            {
                equipmentChanged = true;

                item.refId = it->getCellRef().getRefId();
                item.charge = it->getCellRef().getCharge();
                if (slot == MWWorld::InventoryStore::Slot_CarriedRight)
                {
                    MWMechanics::WeaponType weaptype;
                    MWMechanics::getActiveWeapon(ptrPlayer.getClass().getCreatureStats(ptrPlayer),
                                                 ptrPlayer.getClass().getInventoryStore(ptrPlayer), &weaptype);
                    if (weaptype != MWMechanics::WeapType_Thrown)
                        item.count = 1;
                }
                else
                    item.count = invStore.count(it->getCellRef().getRefId());
            }
        }
        else if (!item.refId.empty())
        {
            equipmentChanged = true;
            item.refId = "";
            item.count = 0;
            item.charge = 0;
        }
    }

    if (equipmentChanged)
    {
        getNetworking()->getPlayerPacket(ID_PLAYER_EQUIPMENT)->setPlayer(this);
        getNetworking()->getPlayerPacket(ID_PLAYER_EQUIPMENT)->Send();
        equipmentChanged = false;
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

    auto setItem = [](Item &item, const MWWorld::Ptr &iter) {
        item.refId = iter.getCellRef().getRefId();
        if (item.refId.find("$dynamic") != string::npos) // skip generated items (self enchanted for e.g.)
            return true;
        item.count = iter.getRefData().getCount();
        item.charge = iter.getCellRef().getCharge();
        return false;
    };

    if (!invChanged)
    {
        for (const auto &itemOld : inventoryChanges.items)
        {
            auto result = ptrInventory.begin();
            for (; result != ptrInventory.end(); ++result)
            {
                if(setItem(item, *result))
                    continue;

                if (item == itemOld)
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
        for (const auto &iter : ptrInventory)
        {
            if(setItem(item, iter))
                continue;

            auto items = inventoryChanges.items;

            if (find(items.begin(), items.end(), item) == items.end())
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

void LocalPlayer::updateAttack()
{
    if (attack.shouldSend)
    {
        if (attack.type == Attack::MAGIC)
        {
            attack.spellId = MWBase::Environment::get().getWindowManager()->getSelectedSpell();

            if (attack.pressed)
                attack.success = MechanicsHelper::getSpellSuccess(attack.spellId, getPlayerPtr());
        }

        getNetworking()->getPlayerPacket(ID_PLAYER_ATTACK)->setPlayer(this);
        getNetworking()->getPlayerPacket(ID_PLAYER_ATTACK)->Send();

        attack.shouldSend = false;
    }
}

void LocalPlayer::updateDeadState(bool forceUpdate)
{
    MWWorld::Ptr ptrPlayer = getPlayerPtr();

    MWMechanics::NpcStats *ptrNpcStats = &ptrPlayer.getClass().getNpcStats(ptrPlayer);
    static bool isDead = false;

    if (ptrNpcStats->isDead() && !isDead)
    {
        creatureStats.mDead = true;

        LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Sending ID_PLAYER_DEATH to server about myself");
        LOG_APPEND(Log::LOG_INFO, "- deathReason was %s", deathReason.c_str());
        getNetworking()->getPlayerPacket(ID_PLAYER_DEATH)->setPlayer(this);
        getNetworking()->getPlayerPacket(ID_PLAYER_DEATH)->Send();
        isDead = true;
    }
    else if (ptrNpcStats->getHealth().getCurrent() > 0 && isDead)
    {
        deathReason = "suicide";
        isDead = false;
    }
}

void LocalPlayer::updateAnimFlags(bool forceUpdate)
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    MWWorld::Ptr ptrPlayer = world->getPlayerPtr();

    MWMechanics::NpcStats ptrNpcStats = ptrPlayer.getClass().getNpcStats(ptrPlayer);
    using namespace MWMechanics;

    static bool wasRunning = ptrNpcStats.getMovementFlag(CreatureStats::Flag_Run);
    static bool wasSneaking = ptrNpcStats.getMovementFlag(CreatureStats::Flag_Sneak);
    static bool wasForceJumping = ptrNpcStats.getMovementFlag(CreatureStats::Flag_ForceJump);
    static bool wasForceMoveJumping = ptrNpcStats.getMovementFlag(CreatureStats::Flag_ForceMoveJump);

    bool isRunning = ptrNpcStats.getMovementFlag(CreatureStats::Flag_Run);
    bool isSneaking = ptrNpcStats.getMovementFlag(CreatureStats::Flag_Sneak);
    bool isForceJumping = ptrNpcStats.getMovementFlag(CreatureStats::Flag_ForceJump);
    bool isForceMoveJumping = ptrNpcStats.getMovementFlag(CreatureStats::Flag_ForceMoveJump);
    
    isFlying = world->isFlying(ptrPlayer);
    bool isJumping = !world->isOnGround(ptrPlayer) && !isFlying;

    // We need to send a new packet at the end of jumping and flying too,
    // so keep track of what we were doing last frame
    static bool wasJumping = false;
    static bool wasFlying = false;

    MWMechanics::DrawState_ currentDrawState = ptrPlayer.getClass().getNpcStats(ptrPlayer).getDrawState();
    static MWMechanics::DrawState_ lastDrawState = ptrPlayer.getClass().getNpcStats(ptrPlayer).getDrawState();

    if (wasRunning != isRunning ||
        wasSneaking != isSneaking || wasForceJumping != isForceJumping ||
        wasForceMoveJumping != isForceMoveJumping || lastDrawState != currentDrawState ||
        wasJumping || isJumping || wasFlying != isFlying ||
        forceUpdate)
    {
        wasSneaking = isSneaking;
        wasRunning = isRunning;
        wasForceJumping = isForceJumping;
        wasForceMoveJumping = isForceMoveJumping;
        lastDrawState = currentDrawState;
        
        wasFlying = isFlying;
        wasJumping = isJumping;

        movementFlags = 0;

#define __SETFLAG(flag, value) (value) ? (movementFlags | flag) : (movementFlags & ~flag)

        movementFlags = __SETFLAG(CreatureStats::Flag_Sneak, isSneaking);
        movementFlags = __SETFLAG(CreatureStats::Flag_Run, isRunning);
        movementFlags = __SETFLAG(CreatureStats::Flag_ForceJump, isForceJumping);
        movementFlags = __SETFLAG(CreatureStats::Flag_ForceJump, isJumping);
        movementFlags = __SETFLAG(CreatureStats::Flag_ForceMoveJump, isForceMoveJumping);

#undef __SETFLAG

        if (currentDrawState == MWMechanics::DrawState_Nothing)
            drawState = 0;
        else if (currentDrawState == MWMechanics::DrawState_Weapon)
            drawState = 1;
        else if (currentDrawState == MWMechanics::DrawState_Spell)
            drawState = 2;

        if (isJumping)
            updatePosition(true); // fix position after jump;

        getNetworking()->getPlayerPacket(ID_PLAYER_ANIM_FLAGS)->setPlayer(this);
        getNetworking()->getPlayerPacket(ID_PLAYER_ANIM_FLAGS)->Send();
    }
}

void LocalPlayer::addItems()
{
    MWWorld::Ptr ptrPlayer = getPlayerPtr();
    MWWorld::ContainerStore &ptrStore = ptrPlayer.getClass().getContainerStore(ptrPlayer);

    for (const auto &item : inventoryChanges.items)
    {
        try
        {
            MWWorld::Ptr itemPtr = *ptrStore.add(item.refId, item.count, ptrPlayer);
            if (item.charge != -1)
                itemPtr.getCellRef().setCharge(item.charge);
        }
        catch (std::exception&)
        {
            LOG_APPEND(Log::LOG_INFO, "- Ignored addition of invalid inventory item %s", item.refId.c_str());
        }
    }
}

void LocalPlayer::addSpells()
{
    MWWorld::Ptr ptrPlayer = getPlayerPtr();
    MWMechanics::Spells &ptrSpells = ptrPlayer.getClass().getCreatureStats(ptrPlayer).getSpells();

    for (const auto &spell : spellbookChanges.spells)
        // Only add spells that are ensured to exist
        if (MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search(spell.mId))
            ptrSpells.add(spell.mId);
        else
            LOG_APPEND(Log::LOG_INFO, "- Ignored addition of invalid spell %s", spell.mId.c_str());
}

void LocalPlayer::addJournalItems()
{
    for (const auto &journalItem : journalChanges.journalItems)
    {
        MWWorld::Ptr ptrFound;

        if (journalItem.type == JournalItem::ENTRY)
        {
            ptrFound = MWBase::Environment::get().getWorld()->searchPtr(journalItem.actorRefId, false);

            if (!ptrFound)
                ptrFound = getPlayerPtr();
        }

        try
        {
            if (journalItem.type == JournalItem::ENTRY)
                MWBase::Environment::get().getJournal()->addEntry(journalItem.quest, journalItem.index, ptrFound);
            else
                MWBase::Environment::get().getJournal()->setJournalIndex(journalItem.quest, journalItem.index);
        }
        catch (std::exception&)
        {
            LOG_APPEND(Log::LOG_INFO, "- Ignored addition of invalid journal quest %s", journalItem.quest.c_str());
        }
    }
}

void LocalPlayer::addTopics()
{
    auto &env = MWBase::Environment::get();
    for (const auto &topic : topicChanges.topics)
    {
        std::string topicId = topic.topicId;

        // If we're using a translated version of Morrowind, translate this topic from English into our language
        if (env.getWindowManager()->getTranslationDataStorage().hasTranslation())
            topicId = env.getWindowManager()->getTranslationDataStorage().getLocalizedTopicId(topicId);

        env.getDialogueManager()->addTopic(topicId);

        if (env.getWindowManager()->containsMode(MWGui::GM_Dialogue))
            env.getDialogueManager()->updateTopics();
    }
}

void LocalPlayer::removeItems()
{
    MWWorld::Ptr ptrPlayer = getPlayerPtr();
    MWWorld::ContainerStore &ptrStore = ptrPlayer.getClass().getContainerStore(ptrPlayer);

    for (const auto &item : inventoryChanges.items)
        ptrStore.remove(item.refId, item.count, ptrPlayer);
}

void LocalPlayer::removeSpells()
{
    MWWorld::Ptr ptrPlayer = getPlayerPtr();
    MWMechanics::Spells &ptrSpells = ptrPlayer.getClass().getCreatureStats(ptrPlayer).getSpells();

    MWBase::WindowManager *wm = MWBase::Environment::get().getWindowManager();
    for (const auto &spell : spellbookChanges.spells)
    {
        ptrSpells.remove(spell.mId);
        if (spell.mId == wm->getSelectedSpell())
            wm->unsetSelectedSpell();
    }
}

void LocalPlayer::setDynamicStats()
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    MWWorld::Ptr ptrPlayer = world->getPlayerPtr();

    MWMechanics::CreatureStats *ptrCreatureStats = &ptrPlayer.getClass().getCreatureStats(ptrPlayer);
    MWMechanics::DynamicStat<float> dynamicStat;

    for (int i = 0; i < 3; ++i)
    {
        dynamicStat = ptrCreatureStats->getDynamic(i);
        dynamicStat.setBase(creatureStats.mDynamic[i].mBase);
        dynamicStat.setCurrent(creatureStats.mDynamic[i].mCurrent);
        ptrCreatureStats->setDynamic(i, dynamicStat);
    }
}

void LocalPlayer::setAttributes()
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    MWWorld::Ptr ptrPlayer = world->getPlayerPtr();

    MWMechanics::CreatureStats *ptrCreatureStats = &ptrPlayer.getClass().getCreatureStats(ptrPlayer);
    MWMechanics::AttributeValue attributeValue;

    for (int i = 0; i < 8; ++i)
    {
        attributeValue.readState(creatureStats.mAttributes[i]);
        ptrCreatureStats->setAttribute(i, attributeValue);
    }
}

void LocalPlayer::setSkills()
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    MWWorld::Ptr ptrPlayer = world->getPlayerPtr();

    MWMechanics::NpcStats *ptrNpcStats = &ptrPlayer.getClass().getNpcStats(ptrPlayer);
    MWMechanics::SkillValue skillValue;

    for (int i = 0; i < 27; ++i)
    {
        skillValue.readState(npcStats.mSkills[i]);
        ptrNpcStats->setSkill(i, skillValue);
    }

    for (int i = 0; i < 8; ++i)
        ptrNpcStats->setSkillIncrease(i, npcStats.mSkillIncrease[i]);

    ptrNpcStats->setLevelProgress(npcStats.mLevelProgress);
}

void LocalPlayer::setLevel()
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    MWWorld::Ptr ptrPlayer = world->getPlayerPtr();

    MWMechanics::CreatureStats *ptrCreatureStats = &ptrPlayer.getClass().getCreatureStats(ptrPlayer);
    ptrCreatureStats->setLevel(creatureStats.mLevel);
}

void LocalPlayer::setBounty()
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    MWWorld::Ptr ptrPlayer = world->getPlayerPtr();

    MWMechanics::NpcStats *ptrNpcStats = &ptrPlayer.getClass().getNpcStats(ptrPlayer);
    ptrNpcStats->setBounty(npcStats.mBounty);
}

void LocalPlayer::setPosition()
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    MWWorld::Ptr ptrPlayer = world->getPlayerPtr();

    // If we're ignoring this position packet because of an invalid cell change,
    // don't make the next one get ignored as well
    if (ignorePosPacket)
        ignorePosPacket = false;
    else
    {
        world->getPlayer().setTeleported(true);

        world->moveObject(ptrPlayer, position.pos[0], position.pos[1], position.pos[2]);
        world->rotateObject(ptrPlayer, position.rot[0], position.rot[1], position.rot[2]);
        world->setInertialForce(ptrPlayer, osg::Vec3f(0.f, 0.f, 0.f));
    }

    updatePosition(true);

    // Make sure we update our draw state, or we'll end up with the wrong one
    updateAnimFlags(true);
}

void LocalPlayer::setCell()
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    MWWorld::Ptr ptrPlayer = world->getPlayerPtr();
    ESM::Position pos;

    // To avoid crashes, close any container menus this player may be in
    if (MWBase::Environment::get().getWindowManager()->containsMode(MWGui::GM_Container))
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(MWGui::GM_Container);
        MWBase::Environment::get().getWindowManager()->setDragDrop(false);
    }

    world->getPlayer().setTeleported(true);

    int x = cell.mData.mX;
    int y = cell.mData.mY;

    if (cell.isExterior())
    {
        world->indexToPosition(x, y, pos.pos[0], pos.pos[1], true);
        pos.pos[2] = 0;

        pos.rot[0] = pos.rot[1] = pos.rot[2] = 0;

        world->changeToExteriorCell(pos, true);
        world->fixPosition(ptrPlayer);
    }
    else if (world->findExteriorPosition(cell.mName, pos))
    {
        world->changeToExteriorCell(pos, true);
        world->fixPosition(ptrPlayer);
    }
    else
    {
        try
        {
            world->findInteriorPosition(cell.mName, pos);
            world->changeToInteriorCell(cell.mName, pos, true);
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
        mwmp::Item &currentItem = equipedItems[slot];

        if (!currentItem.refId.empty())
        {
            auto it = find_if(ptrInventory.begin(), ptrInventory.end(), [&currentItem](const MWWorld::Ptr &a) {
                return Misc::StringUtils::ciEqual(a.getCellRef().getRefId(), currentItem.refId);
            });

            if (it == ptrInventory.end()) // If the item is not in our inventory, add it
            {
                auto equipped = equipedItems[slot];

                try
                {
                    auto addIter = ptrInventory.ContainerStore::add(equipped.refId.c_str(), equipped.count, ptrPlayer);
                    ptrInventory.equip(slot, addIter, ptrPlayer);
                }
                catch (std::exception&)
                {
                    LOG_APPEND(Log::LOG_INFO, "- Ignored addition of invalid equipment item %s", equipped.refId.c_str());
                }
            }
            else
                ptrInventory.equip(slot, it, ptrPlayer);
        }
        else
            ptrInventory.unequipSlot(slot, ptrPlayer);
    }
}

void LocalPlayer::setInventory()
{
    MWWorld::Ptr ptrPlayer = getPlayerPtr();
    MWWorld::ContainerStore &ptrStore = ptrPlayer.getClass().getContainerStore(ptrPlayer);

    // Clear items in inventory
    ptrStore.clear();

    // Proceed by adding items
    addItems();

    // Don't automatically setEquipment() here, or the player could end
    // up getting a new set of their starting clothes, or other items
    // supposed to no longer exist
    //
    // Instead, expect server scripts to do that manually
}

void LocalPlayer::setSpellbook()
{
    MWWorld::Ptr ptrPlayer = getPlayerPtr();
    MWMechanics::Spells &ptrSpells = ptrPlayer.getClass().getCreatureStats(ptrPlayer).getSpells();

    // Clear spells in spellbook, while ignoring abilities, powers, etc.
    while (true)
    {
        MWMechanics::Spells::TIterator iter = ptrSpells.begin();
        for (; iter != ptrSpells.end(); iter++)
        {
            const ESM::Spell *spell = iter->first;
            if (spell->mData.mType == ESM::Spell::ST_Spell)
            {
                ptrSpells.remove(spell->mId);
                break;
            }
        }
        if (iter == ptrSpells.end())
            break;
    }

    // Proceed by adding spells
    addSpells();
}

void LocalPlayer::setFactions()
{
    MWWorld::Ptr ptrPlayer = getPlayerPtr();
    MWMechanics::NpcStats &ptrNpcStats = ptrPlayer.getClass().getNpcStats(ptrPlayer);

    for (const auto &faction : factionChanges.factions)
    {
        // If the player isn't in this faction, make them join it
        if (!ptrNpcStats.isInFaction(faction.factionId))
            ptrNpcStats.joinFaction(faction.factionId);

        if (factionChanges.action == mwmp::FactionChanges::RANK)
        {
            // While the faction rank is different in the packet than in the NpcStats,
            // adjust the NpcStats accordingly
            while (faction.rank != ptrNpcStats.getFactionRanks().at(faction.factionId))
            {
                if (faction.rank > ptrNpcStats.getFactionRanks().at(faction.factionId))
                    ptrNpcStats.raiseRank(faction.factionId);
                else
                    ptrNpcStats.lowerRank(faction.factionId);
            }
        }
        else if (factionChanges.action == mwmp::FactionChanges::EXPULSION)
        {
            // If the expelled state is different in the packet than in the NpcStats,
            // adjust the NpcStats accordingly
            if (faction.isExpelled != ptrNpcStats.getExpelled(faction.factionId))
            {
                if (faction.isExpelled)
                    ptrNpcStats.expell(faction.factionId);
                else
                    ptrNpcStats.clearExpelled(faction.factionId);
            }
        }

        else if (factionChanges.action == mwmp::FactionChanges::REPUTATION)
            ptrNpcStats.setFactionReputation(faction.factionId, faction.reputation);
    }
}

void LocalPlayer::setKills()
{
    for (const auto &kill : killChanges.kills)
        MWBase::Environment::get().getMechanicsManager()->setDeaths(kill.refId, kill.number);
}

void LocalPlayer::setBooks()
{
    MWWorld::Ptr ptrPlayer = getPlayerPtr();
    MWMechanics::NpcStats &ptrNpcStats = ptrPlayer.getClass().getNpcStats(ptrPlayer);

    for (const auto &book : bookChanges.books)
        ptrNpcStats.flagAsUsed(book.bookId);
}

void LocalPlayer::setMapExplored()
{
    MWWorld::Ptr ptrPlayer = getPlayerPtr();
    MWMechanics::NpcStats &ptrNpcStats = ptrPlayer.getClass().getNpcStats(ptrPlayer);

    for (const auto &cellExplored : mapChanges.cellsExplored)
    {
        MWWorld::CellStore *ptrCellStore = Main::get().getCellController()->getCellStore(cellExplored);

        if (ptrCellStore)
            MWBase::Environment::get().getWindowManager()->setCellExplored(ptrCellStore);
    }
}

void LocalPlayer::setShapeshift()
{
    MWWorld::Ptr ptrPlayer = getPlayerPtr();
    MWBase::Environment::get().getMechanicsManager()->setWerewolf(ptrPlayer, isWerewolf);
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

    getNetworking()->getPlayerPacket(ID_PLAYER_CHARCLASS)->setPlayer(this);
    getNetworking()->getPlayerPacket(ID_PLAYER_CHARCLASS)->Send();
}

void LocalPlayer::sendInventory()
{
    MWWorld::Ptr ptrPlayer = getPlayerPtr();
    MWWorld::InventoryStore &ptrInventory = ptrPlayer.getClass().getInventoryStore(ptrPlayer);
    mwmp::Item item;

    inventoryChanges.items.clear();

    for (const auto &iter : ptrInventory)
    {
        item.refId = iter.getCellRef().getRefId();
        if (item.refId.find("$dynamic") != string::npos) // skip generated items (self enchanted for e.g.)
            continue;

        item.count = iter.getRefData().getCount();
        item.charge = iter.getCellRef().getCharge();

        inventoryChanges.items.push_back(item);
    }

    inventoryChanges.count = (unsigned int) inventoryChanges.items.size();
    inventoryChanges.action = InventoryChanges::SET;
    getNetworking()->getPlayerPacket(ID_PLAYER_INVENTORY)->setPlayer(this);
    getNetworking()->getPlayerPacket(ID_PLAYER_INVENTORY)->Send();
}

void LocalPlayer::sendSpellbook()
{
    MWWorld::Ptr ptrPlayer = getPlayerPtr();
    MWMechanics::Spells &ptrSpells = ptrPlayer.getClass().getCreatureStats(ptrPlayer).getSpells();

    spellbookChanges.spells.clear();

    // Send spells in spellbook, while ignoring abilities, powers, etc.
    for (const auto &spell : ptrSpells)
    {
        if (spell.first->mData.mType == ESM::Spell::ST_Spell)
            spellbookChanges.spells.push_back(*spell.first);
    }

    spellbookChanges.action = SpellbookChanges::SET;
    getNetworking()->getPlayerPacket(ID_PLAYER_SPELLBOOK)->setPlayer(this);
    getNetworking()->getPlayerPacket(ID_PLAYER_SPELLBOOK)->Send();
}

void LocalPlayer::sendCellStates()
{
    LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Sending ID_PLAYER_CELL_STATE to server");
    getNetworking()->getPlayerPacket(ID_PLAYER_CELL_STATE)->setPlayer(this);
    getNetworking()->getPlayerPacket(ID_PLAYER_CELL_STATE)->Send();
}

void LocalPlayer::sendSpellAddition(std::string id)
{
    if (id.find("$dynamic") != string::npos) // skip custom spells
        return;

    spellbookChanges.spells.clear();

    ESM::Spell spell;
    spell.mId = id;
    spellbookChanges.spells.push_back(spell);

    spellbookChanges.action = SpellbookChanges::ADD;
    getNetworking()->getPlayerPacket(ID_PLAYER_SPELLBOOK)->setPlayer(this);
    getNetworking()->getPlayerPacket(ID_PLAYER_SPELLBOOK)->Send();
}

void LocalPlayer::sendSpellRemoval(std::string id)
{
    if (id.find("$dynamic") != string::npos) // skip custom spells
        return;

    spellbookChanges.spells.clear();

    ESM::Spell spell;
    spell.mId = id;
    spellbookChanges.spells.push_back(spell);

    spellbookChanges.action = SpellbookChanges::REMOVE;
    getNetworking()->getPlayerPacket(ID_PLAYER_SPELLBOOK)->setPlayer(this);
    getNetworking()->getPlayerPacket(ID_PLAYER_SPELLBOOK)->Send();
}

void LocalPlayer::sendSpellAddition(const ESM::Spell &spell)
{
    /*
    spellbookChanges.spells.clear();

    spellbookChanges.spells.push_back(spell);

    spellbookChanges.action = SpellbookChanges::ADD;
    getNetworking()->getPlayerPacket(ID_PLAYER_SPELLBOOK)->setPlayer(this);
    getNetworking()->getPlayerPacket(ID_PLAYER_SPELLBOOK)->Send();
    */
}

void LocalPlayer::sendSpellRemoval(const ESM::Spell &spell)
{
    /*
    spellbookChanges.spells.clear();

    spellbookChanges.spells.push_back(spell);

    spellbookChanges.action = SpellbookChanges::REMOVE;
    getNetworking()->getPlayerPacket(ID_PLAYER_SPELLBOOK)->setPlayer(this);
    getNetworking()->getPlayerPacket(ID_PLAYER_SPELLBOOK)->Send();
    */
}

void LocalPlayer::sendJournalEntry(const std::string& quest, int index, const MWWorld::Ptr& actor)
{
    journalChanges.journalItems.clear();

    mwmp::JournalItem journalItem;
    journalItem.type = JournalItem::ENTRY;
    journalItem.quest = quest;
    journalItem.index = index;
    journalItem.actorRefId = actor.getCellRef().getRefId();

    journalChanges.journalItems.push_back(journalItem);

    getNetworking()->getPlayerPacket(ID_PLAYER_JOURNAL)->setPlayer(this);
    getNetworking()->getPlayerPacket(ID_PLAYER_JOURNAL)->Send();
}

void LocalPlayer::sendJournalIndex(const std::string& quest, int index)
{
    journalChanges.journalItems.clear();

    mwmp::JournalItem journalItem;
    journalItem.type = JournalItem::INDEX;
    journalItem.quest = quest;
    journalItem.index = index;

    journalChanges.journalItems.push_back(journalItem);

    getNetworking()->getPlayerPacket(ID_PLAYER_JOURNAL)->setPlayer(this);
    getNetworking()->getPlayerPacket(ID_PLAYER_JOURNAL)->Send();
}

void LocalPlayer::sendFactionRank(const std::string& factionId, int rank)
{
    factionChanges.factions.clear();
    factionChanges.action = FactionChanges::RANK;

    mwmp::Faction faction;
    faction.factionId = factionId;
    faction.rank = rank;

    factionChanges.factions.push_back(faction);

    getNetworking()->getPlayerPacket(ID_PLAYER_FACTION)->setPlayer(this);
    getNetworking()->getPlayerPacket(ID_PLAYER_FACTION)->Send();
}

void LocalPlayer::sendFactionExpulsionState(const std::string& factionId, bool isExpelled)
{
    factionChanges.factions.clear();
    factionChanges.action = FactionChanges::EXPULSION;

    mwmp::Faction faction;
    faction.factionId = factionId;
    faction.isExpelled = isExpelled;

    factionChanges.factions.push_back(faction);

    getNetworking()->getPlayerPacket(ID_PLAYER_FACTION)->setPlayer(this);
    getNetworking()->getPlayerPacket(ID_PLAYER_FACTION)->Send();
}

void LocalPlayer::sendFactionReputation(const std::string& factionId, int reputation)
{
    factionChanges.factions.clear();
    factionChanges.action = FactionChanges::REPUTATION;

    mwmp::Faction faction;
    faction.factionId = factionId;
    faction.reputation = reputation;

    factionChanges.factions.push_back(faction);

    getNetworking()->getPlayerPacket(ID_PLAYER_FACTION)->setPlayer(this);
    getNetworking()->getPlayerPacket(ID_PLAYER_FACTION)->Send();
}

void LocalPlayer::sendTopic(const std::string& topicId)
{
    topicChanges.topics.clear();

    mwmp::Topic topic;

    // For translated versions of the game, make sure we translate the topic back into English first
    if (MWBase::Environment::get().getWindowManager()->getTranslationDataStorage().hasTranslation())
        topic.topicId = MWBase::Environment::get().getWindowManager()->getTranslationDataStorage().topicID(topicId);
    else
        topic.topicId = topicId;

    LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Sending ID_PLAYER_TOPIC with topic %s", topic.topicId.c_str());

    topicChanges.topics.push_back(topic);

    getNetworking()->getPlayerPacket(ID_PLAYER_TOPIC)->setPlayer(this);
    getNetworking()->getPlayerPacket(ID_PLAYER_TOPIC)->Send();
}

void LocalPlayer::sendKill(const std::string& refId, int number)
{
    killChanges.kills.clear();

    mwmp::Kill kill;
    kill.refId = refId;
    kill.number = number;

    LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Sending ID_PLAYER_KILL_COUNT with refId %s, number %i", refId.c_str(), number);

    killChanges.kills.push_back(kill);

    getNetworking()->getPlayerPacket(ID_PLAYER_KILL_COUNT)->setPlayer(this);
    getNetworking()->getPlayerPacket(ID_PLAYER_KILL_COUNT)->Send();
}

void LocalPlayer::sendBook(const std::string& bookId)
{
    bookChanges.books.clear();

    mwmp::Book book;
    book.bookId = bookId;

    LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Sending ID_PLAYER_BOOK with book %s", book.bookId.c_str());

    bookChanges.books.push_back(book);

    getNetworking()->getPlayerPacket(ID_PLAYER_BOOK)->setPlayer(this);
    getNetworking()->getPlayerPacket(ID_PLAYER_BOOK)->Send();
}

void LocalPlayer::sendShapeshift(bool werewolfState)
{
    isWerewolf = werewolfState;

    LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Sending ID_PLAYER_SHAPESHIFT with isWerewolf of %s", isWerewolf ? "true" : "false");

    getNetworking()->getPlayerPacket(ID_PLAYER_SHAPESHIFT)->setPlayer(this);
    getNetworking()->getPlayerPacket(ID_PLAYER_SHAPESHIFT)->Send();
}

void LocalPlayer::clearCellStates()
{
    cellStateChanges.cellStates.clear();
}

void LocalPlayer::clearCurrentContainer()
{
    currentContainer.refId = "";
    currentContainer.refNumIndex = 0;
    currentContainer.mpNum = 0;
}

void LocalPlayer::storeCellState(ESM::Cell cell, int stateType)
{
    std::vector<CellState>::iterator iter;

    for (iter = cellStateChanges.cellStates.begin(); iter != cellStateChanges.cellStates.end(); )
    {
        // If there's already a cell state recorded for this particular cell,
        // remove it
        if (cell.getDescription() == (*iter).cell.getDescription())
            iter = cellStateChanges.cellStates.erase(iter);
        else
            ++iter;
    }

    CellState cellState;
    cellState.cell = cell;
    cellState.type = stateType;

    cellStateChanges.cellStates.push_back(cellState);
}

void LocalPlayer::storeCurrentContainer(const MWWorld::Ptr &container, bool loot)
{
    currentContainer.refId = container.getCellRef().getRefId();
    currentContainer.refNumIndex = container.getCellRef().getRefNum().mIndex;
    currentContainer.mpNum = container.getCellRef().getMpNum();
    currentContainer.loot = loot;
}
