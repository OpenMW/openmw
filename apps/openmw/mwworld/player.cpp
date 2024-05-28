#include "player.hpp"

#include <stdexcept>

#include <components/debug/debuglog.hpp>

#include <components/esm/defs.hpp>
#include <components/esm3/esmreader.hpp>
#include <components/esm3/esmwriter.hpp>
#include <components/esm3/loadbsgn.hpp>
#include <components/esm3/loadmgef.hpp>
#include <components/esm3/player.hpp>
#include <components/fallback/fallback.hpp>

#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/magiceffects.hpp"
#include "../mwworld/worldmodel.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/luamanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwmechanics/movement.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/spellutil.hpp"

#include "../mwrender/camera.hpp"
#include "../mwrender/renderingmanager.hpp"

#include "cellstore.hpp"
#include "class.hpp"
#include "ptr.hpp"

namespace MWWorld
{
    Player::Player(const ESM::NPC* player)
        : mCellStore(nullptr)
        , mLastKnownExteriorPosition(0, 0, 0)
        , mMarkedPosition(ESM::Position())
        , mMarkedCell(nullptr)
        , mTeleported(false)
        , mCurrentCrimeId(-1)
        , mPaidCrimeId(-1)
        , mJumping(false)
    {
        ESM::CellRef cellRef;
        cellRef.blank();
        cellRef.mRefID = ESM::RefId::stringRefId("Player");
        mPlayer = LiveCellRef<ESM::NPC>(cellRef, player);

        ESM::Position playerPos = mPlayer.mData.getPosition();
        playerPos.pos[0] = playerPos.pos[1] = playerPos.pos[2] = 0;
        mPlayer.mData.setPosition(playerPos);
    }

    void Player::saveStats()
    {
        MWMechanics::NpcStats& stats = getPlayer().getClass().getNpcStats(getPlayer());

        for (size_t i = 0; i < mSaveSkills.size(); ++i)
            mSaveSkills[i] = stats.getSkill(ESM::Skill::indexToRefId(i)).getModified();
        for (size_t i = 0; i < mSaveAttributes.size(); ++i)
            mSaveAttributes[i] = stats.getAttribute(ESM::Attribute::indexToRefId(i)).getModified();
    }

    void Player::restoreStats()
    {
        const auto& store = MWBase::Environment::get().getESMStore();
        const MWWorld::Store<ESM::GameSetting>& gmst = store->get<ESM::GameSetting>();
        MWMechanics::CreatureStats& creatureStats = getPlayer().getClass().getCreatureStats(getPlayer());
        MWMechanics::NpcStats& npcStats = getPlayer().getClass().getNpcStats(getPlayer());
        MWMechanics::DynamicStat<float> health = creatureStats.getDynamic(0);
        creatureStats.setHealth(health.getBase() / gmst.find("fWereWolfHealth")->mValue.getFloat());
        for (size_t i = 0; i < mSaveSkills.size(); ++i)
        {
            auto& skill = npcStats.getSkill(ESM::Skill::indexToRefId(i));
            skill.restore(skill.getDamage());
            skill.setModifier(mSaveSkills[i] - skill.getBase());
        }
        for (size_t i = 0; i < mSaveAttributes.size(); ++i)
        {
            auto id = ESM::Attribute::indexToRefId(i);
            auto attribute = npcStats.getAttribute(id);
            attribute.restore(attribute.getDamage());
            attribute.setModifier(mSaveAttributes[i] - attribute.getBase());
            npcStats.setAttribute(id, attribute);
        }
    }

    void Player::setWerewolfStats()
    {
        const auto& store = MWBase::Environment::get().getESMStore();
        const MWWorld::Store<ESM::GameSetting>& gmst = store->get<ESM::GameSetting>();
        MWMechanics::CreatureStats& creatureStats = getPlayer().getClass().getCreatureStats(getPlayer());
        MWMechanics::NpcStats& npcStats = getPlayer().getClass().getNpcStats(getPlayer());
        MWMechanics::DynamicStat<float> health = creatureStats.getDynamic(0);
        creatureStats.setHealth(health.getBase() * gmst.find("fWereWolfHealth")->mValue.getFloat());
        for (const auto& attribute : store->get<ESM::Attribute>())
        {
            MWMechanics::AttributeValue value = npcStats.getAttribute(attribute.mId);
            value.setModifier(attribute.mWerewolfValue - value.getModified());
            npcStats.setAttribute(attribute.mId, value);
        }

        for (const auto& skill : store->get<ESM::Skill>())
        {
            // Acrobatics is set separately for some reason.
            if (skill.mId == ESM::Skill::Acrobatics)
                continue;

            MWMechanics::SkillValue& value = npcStats.getSkill(skill.mId);
            value.setModifier(skill.mWerewolfValue - value.getModified());
        }
    }

    void Player::set(const ESM::NPC* player)
    {
        mPlayer.mBase = player;
    }

    void Player::setCell(MWWorld::CellStore* cellStore)
    {
        mCellStore = cellStore;
    }

    MWWorld::Ptr Player::getPlayer()
    {
        MWWorld::Ptr ptr(&mPlayer, mCellStore);
        return ptr;
    }

    MWWorld::ConstPtr Player::getConstPlayer() const
    {
        MWWorld::ConstPtr ptr(&mPlayer, mCellStore);
        return ptr;
    }

    void Player::setBirthSign(const ESM::RefId& sign)
    {
        mSign = sign;
    }

    const ESM::RefId& Player::getBirthSign() const
    {
        return mSign;
    }

    void Player::setDrawState(MWMechanics::DrawState state)
    {
        MWWorld::Ptr ptr = getPlayer();
        ptr.getClass().getNpcStats(ptr).setDrawState(state);
    }

    void Player::yaw(float yaw)
    {
        MWWorld::Ptr ptr = getPlayer();
        ptr.getClass().getMovementSettings(ptr).mRotation[2] += yaw;
    }
    void Player::pitch(float pitch)
    {
        MWWorld::Ptr ptr = getPlayer();
        ptr.getClass().getMovementSettings(ptr).mRotation[0] += pitch;
    }
    void Player::roll(float roll)
    {
        MWWorld::Ptr ptr = getPlayer();
        ptr.getClass().getMovementSettings(ptr).mRotation[1] += roll;
    }

    MWMechanics::DrawState Player::getDrawState()
    {
        MWWorld::Ptr ptr = getPlayer();
        return ptr.getClass().getNpcStats(ptr).getDrawState();
    }

    void Player::activate()
    {
        if (MWBase::Environment::get().getWindowManager()->isGuiMode())
            return;

        MWWorld::Ptr player = getPlayer();
        const MWMechanics::NpcStats& playerStats = player.getClass().getNpcStats(player);
        if (playerStats.isParalyzed() || playerStats.getKnockedDown() || playerStats.isDead())
            return;

        MWWorld::Ptr toActivate = MWBase::Environment::get().getWorld()->getFacedObject();

        if (toActivate.isEmpty())
            return;

        if (!toActivate.getClass().hasToolTip(toActivate))
            return;

        MWBase::Environment::get().getLuaManager()->objectActivated(toActivate, player);
    }

    bool Player::wasTeleported() const
    {
        return mTeleported;
    }

    void Player::setTeleported(bool teleported)
    {
        mTeleported = teleported;
    }

    void Player::setJumping(bool jumping)
    {
        mJumping = jumping;
    }

    bool Player::getJumping() const
    {
        return mJumping;
    }

    bool Player::isInCombat()
    {
        return MWBase::Environment::get().getMechanicsManager()->getActorsFighting(getPlayer()).size() != 0;
    }

    bool Player::enemiesNearby()
    {
        return MWBase::Environment::get().getMechanicsManager()->getEnemiesNearby(getPlayer()).size() != 0;
    }

    void Player::markPosition(CellStore* markedCell, const ESM::Position& markedPosition)
    {
        mMarkedCell = markedCell;
        mMarkedPosition = markedPosition;
    }

    void Player::getMarkedPosition(CellStore*& markedCell, ESM::Position& markedPosition) const
    {
        markedCell = mMarkedCell;
        if (mMarkedCell)
            markedPosition = mMarkedPosition;
    }

    void Player::clear()
    {
        mCellStore = nullptr;
        mSign = ESM::RefId();
        mMarkedCell = nullptr;
        mTeleported = false;
        mJumping = false;
        mCurrentCrimeId = -1;
        mPaidCrimeId = -1;
        mPreviousItems.clear();
        mLastKnownExteriorPosition = osg::Vec3f(0, 0, 0);

        mSaveSkills.fill(0.f);
        mSaveAttributes.fill(0.f);

        mMarkedPosition.pos[0] = 0;
        mMarkedPosition.pos[1] = 0;
        mMarkedPosition.pos[2] = 0;
        mMarkedPosition.rot[0] = 0;
        mMarkedPosition.rot[1] = 0;
        mMarkedPosition.rot[2] = 0;
    }

    void Player::write(ESM::ESMWriter& writer, Loading::Listener& progress) const
    {
        ESM::Player player;

        mPlayer.save(player.mObject);
        player.mCellId = mCellStore->getCell()->getId();

        player.mCurrentCrimeId = mCurrentCrimeId;
        player.mPaidCrimeId = mPaidCrimeId;

        player.mBirthsign = mSign;

        player.mLastKnownExteriorPosition[0] = mLastKnownExteriorPosition.x();
        player.mLastKnownExteriorPosition[1] = mLastKnownExteriorPosition.y();
        player.mLastKnownExteriorPosition[2] = mLastKnownExteriorPosition.z();

        if (mMarkedCell)
        {
            player.mHasMark = true;
            player.mMarkedPosition = mMarkedPosition;
            player.mMarkedCell = mMarkedCell->getCell()->getId();
        }
        else
            player.mHasMark = false;

        for (size_t i = 0; i < mSaveAttributes.size(); ++i)
            player.mSaveAttributes[i] = mSaveAttributes[i];
        for (size_t i = 0; i < mSaveSkills.size(); ++i)
            player.mSaveSkills[i] = mSaveSkills[i];

        player.mPreviousItems = mPreviousItems;

        writer.startRecord(ESM::REC_PLAY);
        player.save(writer);
        writer.endRecord(ESM::REC_PLAY);
    }

    bool Player::readRecord(ESM::ESMReader& reader, uint32_t type)
    {
        if (type == ESM::REC_PLAY)
        {
            ESM::Player player;
            player.load(reader);

            if (!mPlayer.checkState(player.mObject))
            {
                // this is the one object we can not silently drop.
                throw std::runtime_error("invalid player state record (object state)");
            }
            if (reader.getFormatVersion() <= ESM::MaxClearModifiersFormatVersion)
                convertMagicEffects(
                    player.mObject.mCreatureStats, player.mObject.mInventory, &player.mObject.mNpcStats);
            else if (reader.getFormatVersion() <= ESM::MaxOldCreatureStatsFormatVersion)
            {
                convertStats(player.mObject.mCreatureStats);
                convertEnchantmentSlots(player.mObject.mCreatureStats, player.mObject.mInventory);
            }
            else if (reader.getFormatVersion() <= ESM::MaxActiveSpellSlotIndexFormatVersion)
                convertEnchantmentSlots(player.mObject.mCreatureStats, player.mObject.mInventory);

            if (!player.mObject.mEnabled)
            {
                Log(Debug::Warning) << "Warning: Savegame attempted to disable the player.";
                player.mObject.mEnabled = true;
            }

            MWBase::Environment::get().getWorldModel()->deregisterLiveCellRef(mPlayer);
            mPlayer.load(player.mObject);

            for (size_t i = 0; i < mSaveAttributes.size(); ++i)
                mSaveAttributes[i] = player.mSaveAttributes[i];
            for (size_t i = 0; i < mSaveSkills.size(); ++i)
                mSaveSkills[i] = player.mSaveSkills[i];

            if (player.mObject.mNpcStats.mIsWerewolf)
            {
                if (reader.getFormatVersion() <= ESM::MaxOldSkillsAndAttributesFormatVersion)
                {
                    setWerewolfStats();
                    if (player.mSetWerewolfAcrobatics)
                        MWBase::Environment::get().getMechanicsManager()->applyWerewolfAcrobatics(getPlayer());
                }
            }

            getPlayer().getClass().getCreatureStats(getPlayer()).getAiSequence().clear();

            MWBase::World& world = *MWBase::Environment::get().getWorld();

            mCellStore = MWBase::Environment::get().getWorldModel()->findCell(player.mCellId);
            if (mCellStore == nullptr)
                Log(Debug::Warning) << "Player cell " << player.mCellId << " no longer exists";

            if (!player.mBirthsign.empty())
            {
                const ESM::BirthSign* sign = world.getStore().get<ESM::BirthSign>().search(player.mBirthsign);
                if (!sign)
                    throw std::runtime_error("invalid player state record (birthsign does not exist)");
            }

            mCurrentCrimeId = player.mCurrentCrimeId;
            mPaidCrimeId = player.mPaidCrimeId;

            mSign = player.mBirthsign;

            mLastKnownExteriorPosition.x() = player.mLastKnownExteriorPosition[0];
            mLastKnownExteriorPosition.y() = player.mLastKnownExteriorPosition[1];
            mLastKnownExteriorPosition.z() = player.mLastKnownExteriorPosition[2];

            if (player.mHasMark)
            {
                if (!world.getStore().get<ESM::Cell>().search(player.mMarkedCell))
                    player.mHasMark = false; // drop mark silently
            }

            if (player.mHasMark)
            {
                mMarkedPosition = player.mMarkedPosition;
                mMarkedCell = &MWBase::Environment::get().getWorldModel()->getCell(player.mMarkedCell);
            }
            else
            {
                mMarkedCell = nullptr;
            }

            mTeleported = false;

            mPreviousItems = player.mPreviousItems;

            return true;
        }

        return false;
    }

    int Player::getNewCrimeId()
    {
        return ++mCurrentCrimeId;
    }

    void Player::recordCrimeId()
    {
        mPaidCrimeId = mCurrentCrimeId;
    }

    int Player::getCrimeId() const
    {
        return mPaidCrimeId;
    }

    void Player::setPreviousItem(const ESM::RefId& boundItemId, const ESM::RefId& previousItemId)
    {
        mPreviousItems[boundItemId] = previousItemId;
    }

    ESM::RefId Player::getPreviousItem(const ESM::RefId& boundItemId)
    {
        return mPreviousItems[boundItemId];
    }

    void Player::erasePreviousItem(const ESM::RefId& boundItemId)
    {
        mPreviousItems.erase(boundItemId);
    }

    void Player::setSelectedSpell(const ESM::RefId& spellId)
    {
        Ptr player = getPlayer();
        InventoryStore& store = player.getClass().getInventoryStore(player);
        store.setSelectedEnchantItem(store.end());
        int castChance = int(MWMechanics::getSpellSuccessChance(spellId, player));
        MWBase::Environment::get().getWindowManager()->setSelectedSpell(spellId, castChance);
        MWBase::Environment::get().getWindowManager()->updateSpellWindow();
    }

    void Player::update()
    {
        auto player = getPlayer();
        const auto world = MWBase::Environment::get().getWorld();
        const auto rendering = world->getRenderingManager();
        auto& store = world->getStore();
        auto& playerClass = player.getClass();
        const auto windowMgr = MWBase::Environment::get().getWindowManager();

        if (player.getCell()->isExterior())
        {
            ESM::Position pos = player.getRefData().getPosition();
            setLastKnownExteriorPosition(pos.asVec3());
        }

        bool isWerewolf = playerClass.getNpcStats(player).isWerewolf();
        bool isFirstPerson = world->isFirstPerson();
        if (isWerewolf && isFirstPerson)
        {
            float werewolfFov = Fallback::Map::getFloat("General_Werewolf_FOV");
            if (werewolfFov != 0)
                rendering->overrideFieldOfView(werewolfFov);
            windowMgr->setWerewolfOverlay(true);
        }
        else
        {
            rendering->resetFieldOfView();
            windowMgr->setWerewolfOverlay(false);
        }

        // Sink the camera while sneaking
        bool sneaking = playerClass.getCreatureStats(player).getStance(MWMechanics::CreatureStats::Stance_Sneak);
        bool swimming = world->isSwimming(player);
        bool flying = world->isFlying(player);

        static const float i1stPersonSneakDelta
            = store.get<ESM::GameSetting>().find("i1stPersonSneakDelta")->mValue.getFloat();
        if (sneaking && !swimming && !flying)
            rendering->getCamera()->setSneakOffset(i1stPersonSneakDelta);
        else
            rendering->getCamera()->setSneakOffset(0.f);

        int blind = 0;
        const auto& magicEffects = playerClass.getCreatureStats(player).getMagicEffects();
        if (!world->getGodModeState())
            blind = static_cast<int>(magicEffects.getOrDefault(ESM::MagicEffect::Blind).getModifier());
        windowMgr->setBlindness(std::clamp(blind, 0, 100));

        int nightEye = static_cast<int>(magicEffects.getOrDefault(ESM::MagicEffect::NightEye).getMagnitude());
        rendering->setNightEyeFactor(std::min(1.f, (nightEye / 100.f)));
    }

}
