#include "creaturestats.hpp"

#include <algorithm>
#include <type_traits>

#include <components/esm3/creaturestats.hpp>
#include <components/esm3/esmreader.hpp>
#include <components/esm3/esmwriter.hpp>
#include <components/esm3/loadmgef.hpp>

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/player.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

namespace MWMechanics
{
    int CreatureStats::sActorId = 0;

    CreatureStats::CreatureStats()
        : mDrawState(DrawState::Nothing)
        , mDead(false)
        , mDeathAnimationFinished(false)
        , mDied(false)
        , mMurdered(false)
        , mFriendlyHits(0)
        , mTalkedTo(false)
        , mAlarmed(false)
        , mAttacked(false)
        , mKnockdown(false)
        , mKnockdownOneFrame(false)
        , mKnockdownOverOneFrame(false)
        , mHitRecovery(false)
        , mBlock(false)
        , mMovementFlags(0)
        , mFallHeight(0)
        , mLastRestock(0, 0)
        , mGoldPool(0)
        , mActorId(-1)
        , mHitAttemptActorId(-1)
        , mDeathAnimation(-1)
        , mTimeOfDeath()
        , mSideMovementAngle(0)
        , mLevel(0)
        , mAttackingOrSpell(false)
        , mAttackType("")
    {
        for (const ESM::Attribute& attribute : MWBase::Environment::get().getESMStore()->get<ESM::Attribute>())
        {
            mAttributes.emplace(attribute.mId, AttributeValue{});
        }
    }

    const AiSequence& CreatureStats::getAiSequence() const
    {
        return mAiSequence;
    }

    AiSequence& CreatureStats::getAiSequence()
    {
        return mAiSequence;
    }

    float CreatureStats::getFatigueTerm() const
    {
        float max = getFatigue().getModified();
        float current = getFatigue().getCurrent();

        float normalised = std::floor(max) == 0 ? 1 : std::max(0.0f, current / max);

        const MWWorld::Store<ESM::GameSetting>& gmst
            = MWBase::Environment::get().getESMStore()->get<ESM::GameSetting>();

        static const float fFatigueBase = gmst.find("fFatigueBase")->mValue.getFloat();
        static const float fFatigueMult = gmst.find("fFatigueMult")->mValue.getFloat();

        return fFatigueBase - fFatigueMult * (1 - normalised);
    }

    const AttributeValue& CreatureStats::getAttribute(ESM::RefId id) const
    {
        return mAttributes.at(id);
    }

    const DynamicStat<float>& CreatureStats::getHealth() const
    {
        return mDynamic[0];
    }

    const DynamicStat<float>& CreatureStats::getMagicka() const
    {
        return mDynamic[1];
    }

    const DynamicStat<float>& CreatureStats::getFatigue() const
    {
        return mDynamic[2];
    }

    const Spells& CreatureStats::getSpells() const
    {
        return mSpells;
    }

    const ActiveSpells& CreatureStats::getActiveSpells() const
    {
        return mActiveSpells;
    }

    const MagicEffects& CreatureStats::getMagicEffects() const
    {
        return mMagicEffects;
    }

    int CreatureStats::getLevel() const
    {
        return mLevel;
    }

    Stat<int> CreatureStats::getAiSetting(AiSetting index) const
    {
        return mAiSettings[static_cast<std::underlying_type_t<AiSetting>>(index)];
    }

    const DynamicStat<float>& CreatureStats::getDynamic(int index) const
    {
        if (index < 0 || index > 2)
        {
            throw std::runtime_error("dynamic stat index is out of range");
        }
        return mDynamic[index];
    }

    Spells& CreatureStats::getSpells()
    {
        return mSpells;
    }

    ActiveSpells& CreatureStats::getActiveSpells()
    {
        return mActiveSpells;
    }

    MagicEffects& CreatureStats::getMagicEffects()
    {
        return mMagicEffects;
    }

    void CreatureStats::setAttribute(ESM::RefId id, float base)
    {
        AttributeValue current = getAttribute(id);
        current.setBase(base);
        setAttribute(id, current);
    }

    void CreatureStats::setAttribute(ESM::RefId id, const AttributeValue& value)
    {
        const AttributeValue& currentValue = mAttributes.at(id);

        if (value != currentValue)
        {
            mAttributes[id] = value;

            if (id == ESM::Attribute::Intelligence)
                recalculateMagicka();
            else if (id == ESM::Attribute::Strength || id == ESM::Attribute::Willpower || id == ESM::Attribute::Agility
                || id == ESM::Attribute::Endurance)
            {
                float strength = getAttribute(ESM::Attribute::Strength).getModified();
                float willpower = getAttribute(ESM::Attribute::Willpower).getModified();
                float agility = getAttribute(ESM::Attribute::Agility).getModified();
                float endurance = getAttribute(ESM::Attribute::Endurance).getModified();
                DynamicStat<float> fatigue = getFatigue();
                float currentToBaseRatio = fatigue.getBase() > 0 ? (fatigue.getCurrent() / fatigue.getBase()) : 0;
                fatigue.setBase(std::max(0.f, strength + willpower + agility + endurance));
                fatigue.setCurrent(fatigue.getBase() * currentToBaseRatio, false, true);
                setFatigue(fatigue);
            }
        }
    }

    void CreatureStats::setHealth(const DynamicStat<float>& value)
    {
        setDynamic(0, value);
    }

    void CreatureStats::setMagicka(const DynamicStat<float>& value)
    {
        setDynamic(1, value);
    }

    void CreatureStats::setFatigue(const DynamicStat<float>& value)
    {
        setDynamic(2, value);
    }

    void CreatureStats::setDynamic(int index, const DynamicStat<float>& value)
    {
        if (index < 0 || index > 2)
            throw std::runtime_error("dynamic stat index is out of range");

        mDynamic[index] = value;

        if (index == 0 && mDynamic[index].getCurrent() < 1)
        {
            if (!mDead)
                mTimeOfDeath = MWBase::Environment::get().getWorld()->getTimeStamp();

            mDead = true;

            mDynamic[index].setCurrent(0);
        }
    }

    void CreatureStats::setLevel(int level)
    {
        mLevel = level;
    }

    void CreatureStats::modifyMagicEffects(const MagicEffects& effects)
    {
        bool recalc = effects.getOrDefault(ESM::MagicEffect::FortifyMaximumMagicka).getModifier()
            != mMagicEffects.getOrDefault(ESM::MagicEffect::FortifyMaximumMagicka).getModifier();
        mMagicEffects.setModifiers(effects);
        if (recalc)
            recalculateMagicka();
    }

    void CreatureStats::setAiSetting(AiSetting index, Stat<int> value)
    {
        mAiSettings[static_cast<std::underlying_type_t<AiSetting>>(index)] = value;
    }

    void CreatureStats::setAiSetting(AiSetting index, int base)
    {
        Stat<int> stat = getAiSetting(index);
        stat.setBase(base);
        setAiSetting(index, stat);
    }

    bool CreatureStats::isParalyzed() const
    {
        MWBase::World* world = MWBase::Environment::get().getWorld();
        const MWWorld::Ptr player = world->getPlayerPtr();
        if (world->getGodModeState() && this == &player.getClass().getCreatureStats(player))
            return false;

        return mMagicEffects.getOrDefault(ESM::MagicEffect::Paralyze).getMagnitude() > 0;
    }

    bool CreatureStats::isDead() const
    {
        return mDead;
    }

    bool CreatureStats::isDeathAnimationFinished() const
    {
        return mDeathAnimationFinished;
    }

    void CreatureStats::setDeathAnimationFinished(bool finished)
    {
        mDeathAnimationFinished = finished;
    }

    void CreatureStats::notifyDied()
    {
        mDied = true;
    }

    bool CreatureStats::hasDied() const
    {
        return mDied;
    }

    void CreatureStats::clearHasDied()
    {
        mDied = false;
    }

    bool CreatureStats::hasBeenMurdered() const
    {
        return mMurdered;
    }

    void CreatureStats::notifyMurder()
    {
        mMurdered = true;
    }

    void CreatureStats::clearHasBeenMurdered()
    {
        mMurdered = false;
    }

    void CreatureStats::resurrect()
    {
        if (mDead)
        {
            mDynamic[0].setCurrent(mDynamic[0].getBase());
            mDead = false;
            mDeathAnimationFinished = false;
        }
    }

    bool CreatureStats::hasCommonDisease() const
    {
        return mSpells.hasCommonDisease();
    }

    bool CreatureStats::hasBlightDisease() const
    {
        return mSpells.hasBlightDisease();
    }

    int CreatureStats::getFriendlyHits() const
    {
        return mFriendlyHits;
    }

    void CreatureStats::friendlyHit()
    {
        ++mFriendlyHits;
    }

    void CreatureStats::resetFriendlyHits()
    {
        mFriendlyHits = 0;
    }

    bool CreatureStats::hasTalkedToPlayer() const
    {
        return mTalkedTo;
    }

    void CreatureStats::talkedToPlayer()
    {
        mTalkedTo = true;
    }

    bool CreatureStats::isAlarmed() const
    {
        return mAlarmed;
    }

    void CreatureStats::setAlarmed(bool alarmed)
    {
        mAlarmed = alarmed;
    }

    bool CreatureStats::getAttacked() const
    {
        return mAttacked;
    }

    void CreatureStats::setAttacked(bool attacked)
    {
        mAttacked = attacked;
    }

    float CreatureStats::getEvasion() const
    {
        float evasion = (getAttribute(ESM::Attribute::Agility).getModified() / 5.0f)
            + (getAttribute(ESM::Attribute::Luck).getModified() / 10.0f);
        evasion *= getFatigueTerm();
        evasion += std::min(100.f, mMagicEffects.getOrDefault(ESM::MagicEffect::Sanctuary).getMagnitude());

        return evasion;
    }

    void CreatureStats::setLastHitObject(const ESM::RefId& objectid)
    {
        mLastHitObject = objectid;
    }

    void CreatureStats::clearLastHitObject()
    {
        mLastHitObject = ESM::RefId();
    }

    const ESM::RefId& CreatureStats::getLastHitObject() const
    {
        return mLastHitObject;
    }

    void CreatureStats::setLastHitAttemptObject(const ESM::RefId& objectid)
    {
        mLastHitAttemptObject = objectid;
    }

    void CreatureStats::clearLastHitAttemptObject()
    {
        mLastHitAttemptObject = ESM::RefId();
    }

    const ESM::RefId& CreatureStats::getLastHitAttemptObject() const
    {
        return mLastHitAttemptObject;
    }

    void CreatureStats::setHitAttemptActorId(int actorId)
    {
        mHitAttemptActorId = actorId;
    }

    int CreatureStats::getHitAttemptActorId() const
    {
        return mHitAttemptActorId;
    }

    void CreatureStats::addToFallHeight(float height)
    {
        mFallHeight += height;
    }

    float CreatureStats::getFallHeight() const
    {
        return mFallHeight;
    }

    float CreatureStats::land(bool isPlayer)
    {
        if (isPlayer)
            MWBase::Environment::get().getWorld()->getPlayer().setJumping(false);

        float height = mFallHeight;
        mFallHeight = 0;
        return height;
    }

    void CreatureStats::recalculateMagicka()
    {
        auto world = MWBase::Environment::get().getWorld();
        float intelligence = getAttribute(ESM::Attribute::Intelligence).getModified();

        float base = 1.f;
        const auto& player = world->getPlayerPtr();
        if (this == &player.getClass().getCreatureStats(player))
            base = world->getStore().get<ESM::GameSetting>().find("fPCbaseMagickaMult")->mValue.getFloat();
        else
            base = world->getStore().get<ESM::GameSetting>().find("fNPCbaseMagickaMult")->mValue.getFloat();

        double magickaFactor = base
            + mMagicEffects.getOrDefault(EffectKey(ESM::MagicEffect::FortifyMaximumMagicka)).getMagnitude() * 0.1;

        DynamicStat<float> magicka = getMagicka();
        float currentToBaseRatio = magicka.getBase() > 0 ? magicka.getCurrent() / magicka.getBase() : 0;
        magicka.setBase(magickaFactor * intelligence);
        magicka.setCurrent(magicka.getBase() * currentToBaseRatio, false, true);
        setMagicka(magicka);
    }

    void CreatureStats::setKnockedDown(bool value)
    {
        mKnockdown = value;
        if (!value) // Resets the "OverOneFrame" flag
            setKnockedDownOverOneFrame(false);
    }

    bool CreatureStats::getKnockedDown() const
    {
        return mKnockdown;
    }

    void CreatureStats::setKnockedDownOneFrame(bool value)
    {
        mKnockdownOneFrame = value;
    }

    bool CreatureStats::getKnockedDownOneFrame() const
    {
        return mKnockdownOneFrame;
    }

    void CreatureStats::setKnockedDownOverOneFrame(bool value)
    {
        mKnockdownOverOneFrame = value;
    }
    bool CreatureStats::getKnockedDownOverOneFrame() const
    {
        return mKnockdownOverOneFrame;
    }

    void CreatureStats::setHitRecovery(bool value)
    {
        mHitRecovery = value;
    }

    bool CreatureStats::getHitRecovery() const
    {
        return mHitRecovery;
    }

    void CreatureStats::setBlock(bool value)
    {
        mBlock = value;
    }

    bool CreatureStats::getBlock() const
    {
        return mBlock;
    }

    bool CreatureStats::getMovementFlag(Flag flag) const
    {
        return (mMovementFlags & flag) != 0;
    }

    void CreatureStats::setMovementFlag(Flag flag, bool state)
    {
        if (state)
            mMovementFlags |= flag;
        else
            mMovementFlags &= ~flag;
    }

    bool CreatureStats::getStance(Stance flag) const
    {
        switch (flag)
        {
            case Stance_Run:
                return getMovementFlag(Flag_Run) || getMovementFlag(Flag_ForceRun);
            case Stance_Sneak:
                return getMovementFlag(Flag_Sneak) || getMovementFlag(Flag_ForceSneak);
            default:
                return false;
        }
    }

    DrawState CreatureStats::getDrawState() const
    {
        return mDrawState;
    }

    void CreatureStats::setDrawState(DrawState state)
    {
        mDrawState = state;
    }

    void CreatureStats::writeState(ESM::CreatureStats& state) const
    {
        for (size_t i = 0; i < state.mAttributes.size(); ++i)
            getAttribute(ESM::Attribute::indexToRefId(i)).writeState(state.mAttributes[i]);

        for (size_t i = 0; i < state.mDynamic.size(); ++i)
            mDynamic[i].writeState(state.mDynamic[i]);

        state.mTradeTime = mLastRestock.toEsm();
        state.mGoldPool = mGoldPool;

        state.mDead = mDead;
        state.mDeathAnimationFinished = mDeathAnimationFinished;
        state.mDied = mDied;
        state.mMurdered = mMurdered;
        // The vanilla engine does not store friendly hits in the save file. Since there's no other mechanism
        // that ever resets the friendly hits (at least not to my knowledge) this should be regarded a feature
        // rather than a bug.
        // state.mFriendlyHits = mFriendlyHits;
        state.mTalkedTo = mTalkedTo;
        state.mAlarmed = mAlarmed;
        state.mAttacked = mAttacked;
        // TODO: rewrite. does this really need 3 separate bools?
        state.mKnockdown = mKnockdown;
        state.mKnockdownOneFrame = mKnockdownOneFrame;
        state.mKnockdownOverOneFrame = mKnockdownOverOneFrame;
        state.mHitRecovery = mHitRecovery;
        state.mBlock = mBlock;
        state.mMovementFlags = mMovementFlags;
        state.mFallHeight = mFallHeight; // TODO: vertical velocity (move from PhysicActor to CreatureStats?)
        state.mLastHitObject = mLastHitObject;
        state.mLastHitAttemptObject = mLastHitAttemptObject;
        state.mRecalcDynamicStats = false;
        state.mDrawState = static_cast<int>(mDrawState);
        state.mLevel = mLevel;
        state.mActorId = mActorId;
        state.mDeathAnimation = mDeathAnimation;
        state.mTimeOfDeath = mTimeOfDeath.toEsm();
        // state.mHitAttemptActorId = mHitAttemptActorId;

        mSpells.writeState(state.mSpells);
        mActiveSpells.writeState(state.mActiveSpells);
        mAiSequence.writeState(state.mAiSequence);
        mMagicEffects.writeState(state.mMagicEffects);

        state.mSummonedCreatures = mSummonedCreatures;
        state.mSummonGraveyard = mSummonGraveyard;

        state.mHasAiSettings = true;
        for (size_t i = 0; i < state.mAiSettings.size(); ++i)
            mAiSettings[i].writeState(state.mAiSettings[i]);

        state.mMissingACDT = false;
    }

    void CreatureStats::readState(const ESM::CreatureStats& state)
    {
        if (!state.mMissingACDT)
        {
            for (size_t i = 0; i < state.mAttributes.size(); ++i)
                mAttributes[ESM::Attribute::indexToRefId(i)].readState(state.mAttributes[i]);

            for (size_t i = 0; i < state.mDynamic.size(); ++i)
                mDynamic[i].readState(state.mDynamic[i]);

            mGoldPool = state.mGoldPool;
            mTalkedTo = state.mTalkedTo;
            mAttacked = state.mAttacked;
        }

        mLastRestock = MWWorld::TimeStamp(state.mTradeTime);

        mDead = state.mDead;
        mDeathAnimationFinished = state.mDeathAnimationFinished;
        mDied = state.mDied;
        mMurdered = state.mMurdered;
        mAlarmed = state.mAlarmed;
        // TODO: rewrite. does this really need 3 separate bools?
        mKnockdown = state.mKnockdown;
        mKnockdownOneFrame = state.mKnockdownOneFrame;
        mKnockdownOverOneFrame = state.mKnockdownOverOneFrame;
        mHitRecovery = state.mHitRecovery;
        mBlock = state.mBlock;
        mMovementFlags = state.mMovementFlags;
        mFallHeight = state.mFallHeight;
        mLastHitObject = state.mLastHitObject;
        mLastHitAttemptObject = state.mLastHitAttemptObject;
        mDrawState = DrawState(state.mDrawState);
        mLevel = state.mLevel;
        mActorId = state.mActorId;
        mDeathAnimation = state.mDeathAnimation;
        mTimeOfDeath = MWWorld::TimeStamp(state.mTimeOfDeath);
        // mHitAttemptActorId = state.mHitAttemptActorId;

        mSpells.readState(state.mSpells, this);
        mActiveSpells.readState(state.mActiveSpells);
        mAiSequence.readState(state.mAiSequence);
        mMagicEffects.readState(state.mMagicEffects);

        mSummonedCreatures = state.mSummonedCreatures;
        mSummonGraveyard = state.mSummonGraveyard;

        if (state.mHasAiSettings)
            for (size_t i = 0; i < state.mAiSettings.size(); ++i)
                mAiSettings[i].readState(state.mAiSettings[i]);
        if (state.mRecalcDynamicStats)
            recalculateMagicka();
    }

    void CreatureStats::setLastRestockTime(MWWorld::TimeStamp tradeTime)
    {
        mLastRestock = tradeTime;
    }

    MWWorld::TimeStamp CreatureStats::getLastRestockTime() const
    {
        return mLastRestock;
    }

    void CreatureStats::setGoldPool(int pool)
    {
        mGoldPool = pool;
    }
    int CreatureStats::getGoldPool() const
    {
        return mGoldPool;
    }

    int CreatureStats::getActorId()
    {
        if (mActorId == -1)
            mActorId = sActorId++;

        return mActorId;
    }

    bool CreatureStats::matchesActorId(int id) const
    {
        return mActorId != -1 && id == mActorId;
    }

    void CreatureStats::cleanup()
    {
        sActorId = 0;
    }

    void CreatureStats::writeActorIdCounter(ESM::ESMWriter& esm)
    {
        esm.startRecord(ESM::REC_ACTC);
        esm.writeHNT("COUN", sActorId);
        esm.endRecord(ESM::REC_ACTC);
    }

    void CreatureStats::readActorIdCounter(ESM::ESMReader& esm)
    {
        esm.getHNT(sActorId, "COUN");
    }

    signed char CreatureStats::getDeathAnimation() const
    {
        return mDeathAnimation;
    }

    void CreatureStats::setDeathAnimation(signed char index)
    {
        mDeathAnimation = index;
    }

    MWWorld::TimeStamp CreatureStats::getTimeOfDeath() const
    {
        return mTimeOfDeath;
    }

    std::multimap<int, int>& CreatureStats::getSummonedCreatureMap()
    {
        return mSummonedCreatures;
    }

    std::vector<int>& CreatureStats::getSummonedCreatureGraveyard()
    {
        return mSummonGraveyard;
    }
}
