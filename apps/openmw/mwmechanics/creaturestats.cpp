#include "creaturestats.hpp"

#include <algorithm>

#include <components/esm/creaturestats.hpp>
#include <components/esm/esmreader.hpp>
#include <components/esm/esmwriter.hpp>

#include "../mwworld/esmstore.hpp"
#include "../mwworld/player.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/mechanicsmanager.hpp"

namespace MWMechanics
{
    int CreatureStats::sActorId = 0;

    CreatureStats::CreatureStats()
        : mDrawState (DrawState_Nothing), mDead (false), mDeathAnimationFinished(false), mDied (false), mMurdered(false), mFriendlyHits (0),
          mTalkedTo (false), mAlarmed (false), mAttacked (false),
          mKnockdown(false), mKnockdownOneFrame(false), mKnockdownOverOneFrame(false),
          mHitRecovery(false), mBlock(false), mMovementFlags(0),
          mFallHeight(0), mRecalcMagicka(false), mLastRestock(0,0), mGoldPool(0), mActorId(-1), mHitAttemptActorId(-1),
          mDeathAnimation(-1), mTimeOfDeath(), mGreetingState(Greet_None),
          mGreetingTimer(0), mTargetAngleRadians(0), mIsTurningToPlayer(false), mLevel (0)
    {
        for (int i=0; i<4; ++i)
            mAiSettings[i] = 0;
    }

    int MWMechanics::CreatureStats::getGreetingTimer() const
    {
        return mGreetingTimer;
    }

    void MWMechanics::CreatureStats::setGreetingTimer(int timer)
    {
        mGreetingTimer = timer;
    }

    float MWMechanics::CreatureStats::getAngleToPlayer() const
    {
        return mTargetAngleRadians;
    }

    void MWMechanics::CreatureStats::setAngleToPlayer(float angle)
    {
        mTargetAngleRadians = angle;
    }

    GreetingState MWMechanics::CreatureStats::getGreetingState() const
    {
        return mGreetingState;
    }

    void MWMechanics::CreatureStats::setGreetingState(GreetingState state)
    {
        mGreetingState = state;
    }

    bool MWMechanics::CreatureStats::isTurningToPlayer() const
    {
        return mIsTurningToPlayer;
    }

    void MWMechanics::CreatureStats::setTurningToPlayer(bool turning)
    {
        mIsTurningToPlayer = turning;
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

        float normalised = floor(max) == 0 ? 1 : std::max (0.0f, current / max);

        const MWWorld::Store<ESM::GameSetting> &gmst =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        static const float fFatigueBase = gmst.find("fFatigueBase")->mValue.getFloat();
        static const float fFatigueMult = gmst.find("fFatigueMult")->mValue.getFloat();

        return fFatigueBase - fFatigueMult * (1-normalised);
    }

    const AttributeValue &CreatureStats::getAttribute(int index) const
    {
        if (index < 0 || index > 7) {
            throw std::runtime_error("attribute index is out of range");
        }
        return mAttributes[index];
    }

    const DynamicStat<float> &CreatureStats::getHealth() const
    {
        return mDynamic[0];
    }

    const DynamicStat<float> &CreatureStats::getMagicka() const
    {
        return mDynamic[1];
    }

    const DynamicStat<float> &CreatureStats::getFatigue() const
    {
        return mDynamic[2];
    }

    const Spells &CreatureStats::getSpells() const
    {
        return mSpells;
    }

    const ActiveSpells &CreatureStats::getActiveSpells() const
    {
        return mActiveSpells;
    }

    const MagicEffects &CreatureStats::getMagicEffects() const
    {
        return mMagicEffects;
    }

    int CreatureStats::getLevel() const
    {
        return mLevel;
    }

    Stat<int> CreatureStats::getAiSetting (AiSetting index) const
    {
        return mAiSettings[index];
    }

    const DynamicStat<float> &CreatureStats::getDynamic(int index) const
    {
        if (index < 0 || index > 2) {
            throw std::runtime_error("dynamic stat index is out of range");
        }
        return mDynamic[index];
    }

    Spells &CreatureStats::getSpells()
    {
        return mSpells;
    }

    ActiveSpells &CreatureStats::getActiveSpells()
    {
        return mActiveSpells;
    }

    MagicEffects &CreatureStats::getMagicEffects()
    {
        return mMagicEffects;
    }

    void CreatureStats::setAttribute(int index, int base)
    {
        AttributeValue current = getAttribute(index);
        current.setBase(base);
        setAttribute(index, current);
    }

    void CreatureStats::setAttribute(int index, const AttributeValue &value)
    {
        if (index < 0 || index > 7) {
            throw std::runtime_error("attribute index is out of range");
        }

        const AttributeValue& currentValue = mAttributes[index];

        if (value != currentValue)
        {
            mAttributes[index] = value;

            if (index == ESM::Attribute::Intelligence)
                mRecalcMagicka = true;
            else if (index == ESM::Attribute::Strength ||
                     index == ESM::Attribute::Willpower ||
                     index == ESM::Attribute::Agility ||
                     index == ESM::Attribute::Endurance)
            {
                int strength     = getAttribute(ESM::Attribute::Strength).getModified();
                int willpower    = getAttribute(ESM::Attribute::Willpower).getModified();
                int agility      = getAttribute(ESM::Attribute::Agility).getModified();
                int endurance    = getAttribute(ESM::Attribute::Endurance).getModified();
                DynamicStat<float> fatigue = getFatigue();
                float diff = (strength+willpower+agility+endurance) - fatigue.getBase();
                float currentToBaseRatio = fatigue.getBase() > 0 ? (fatigue.getCurrent() / fatigue.getBase()) : 0;
                fatigue.setModified(fatigue.getModified() + diff, 0);
                fatigue.setCurrent(fatigue.getBase() * currentToBaseRatio);
                setFatigue(fatigue);
            }
        }
    }

    void CreatureStats::setHealth(const DynamicStat<float> &value)
    {
        setDynamic (0, value);
    }

    void CreatureStats::setMagicka(const DynamicStat<float> &value)
    {
        setDynamic (1, value);
    }

    void CreatureStats::setFatigue(const DynamicStat<float> &value)
    {
        setDynamic (2, value);
    }

    void CreatureStats::setDynamic (int index, const DynamicStat<float> &value)
    {
        if (index < 0 || index > 2)
            throw std::runtime_error("dynamic stat index is out of range");

        mDynamic[index] = value;

        if (index==0 && mDynamic[index].getCurrent()<1)
        {
            if (!mDead)
                mTimeOfDeath = MWBase::Environment::get().getWorld()->getTimeStamp();

            mDead = true;

            mDynamic[index].setModifier(0);
            mDynamic[index].setCurrentModifier(0);
            mDynamic[index].setCurrent(0);
        }
    }

    void CreatureStats::setLevel(int level)
    {
        mLevel = level;
    }

    void CreatureStats::modifyMagicEffects(const MagicEffects &effects)
    {
        if (effects.get(ESM::MagicEffect::FortifyMaximumMagicka).getModifier()
                != mMagicEffects.get(ESM::MagicEffect::FortifyMaximumMagicka).getModifier())
            mRecalcMagicka = true;

        mMagicEffects.setModifiers(effects);
    }

    void CreatureStats::setAiSetting (AiSetting index, Stat<int> value)
    {
        mAiSettings[index] = value;
    }

    void CreatureStats::setAiSetting (AiSetting index, int base)
    {
        Stat<int> stat = getAiSetting(index);
        stat.setBase(base);
        setAiSetting(index, stat);
    }

    bool CreatureStats::isParalyzed() const
    {
        return mMagicEffects.get(ESM::MagicEffect::Paralyze).getMagnitude() > 0;
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
            if (mDynamic[0].getModified() < 1)
                mDynamic[0].setModified(1, 0);

            mDynamic[0].setCurrent(mDynamic[0].getModified());
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

    void CreatureStats::setAlarmed (bool alarmed)
    {
        mAlarmed = alarmed;
    }

    bool CreatureStats::getAttacked() const
    {
        return mAttacked;
    }

    void CreatureStats::setAttacked (bool attacked)
    {
        mAttacked = attacked;
    }

    float CreatureStats::getEvasion() const
    {
        float evasion = (getAttribute(ESM::Attribute::Agility).getModified() / 5.0f) +
                        (getAttribute(ESM::Attribute::Luck).getModified() / 10.0f);
        evasion *= getFatigueTerm();
        evasion += std::min(100.f, mMagicEffects.get(ESM::MagicEffect::Sanctuary).getMagnitude());

        return evasion;
    }

    void CreatureStats::setLastHitObject(const std::string& objectid)
    {
        mLastHitObject = objectid;
    }

    const std::string &CreatureStats::getLastHitObject() const
    {
        return mLastHitObject;
    }

    void CreatureStats::setLastHitAttemptObject(const std::string& objectid)
    {
        mLastHitAttemptObject = objectid;
    }

    const std::string &CreatureStats::getLastHitAttemptObject() const
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

    bool CreatureStats::needToRecalcDynamicStats()
    {
         if (mRecalcMagicka)
         {
             mRecalcMagicka = false;
             return true;
         }
         return false;
    }

    void CreatureStats::setNeedRecalcDynamicStats(bool val)
    {
        mRecalcMagicka = val;
    }

    void CreatureStats::setKnockedDown(bool value)
    {
        mKnockdown = value;
        if(!value) //Resets the "OverOneFrame" flag
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

    void CreatureStats::setKnockedDownOverOneFrame(bool value) {
        mKnockdownOverOneFrame = value;
    }
    bool CreatureStats::getKnockedDownOverOneFrame() const {
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

    bool CreatureStats::getMovementFlag (Flag flag) const
    {
        return (mMovementFlags & flag) != 0;
    }

    void CreatureStats::setMovementFlag (Flag flag, bool state)
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
                return getMovementFlag (Flag_Run) || getMovementFlag (Flag_ForceRun);
            case Stance_Sneak:
                return getMovementFlag (Flag_Sneak) || getMovementFlag (Flag_ForceSneak);
            default:
                return false;
        }
    }

    DrawState_ CreatureStats::getDrawState() const
    {
        return mDrawState;
    }

    void CreatureStats::setDrawState(DrawState_ state)
    {
        mDrawState = state;
    }

    void CreatureStats::writeState (ESM::CreatureStats& state) const
    {
        for (int i=0; i<ESM::Attribute::Length; ++i)
            mAttributes[i].writeState (state.mAttributes[i]);

        for (int i=0; i<3; ++i)
            mDynamic[i].writeState (state.mDynamic[i]);

        state.mTradeTime = mLastRestock.toEsm();
        state.mGoldPool = mGoldPool;

        state.mDead = mDead;
        state.mDeathAnimationFinished = mDeathAnimationFinished;
        state.mDied = mDied;
        state.mMurdered = mMurdered;
        // The vanilla engine does not store friendly hits in the save file. Since there's no other mechanism
        // that ever resets the friendly hits (at least not to my knowledge) this should be regarded a feature
        // rather than a bug.
        //state.mFriendlyHits = mFriendlyHits;
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
        state.mRecalcDynamicStats = mRecalcMagicka;
        state.mDrawState = mDrawState;
        state.mLevel = mLevel;
        state.mActorId = mActorId;
        state.mDeathAnimation = mDeathAnimation;
        state.mTimeOfDeath = mTimeOfDeath.toEsm();
        //state.mHitAttemptActorId = mHitAttemptActorId;

        mSpells.writeState(state.mSpells);
        mActiveSpells.writeState(state.mActiveSpells);
        mAiSequence.writeState(state.mAiSequence);
        mMagicEffects.writeState(state.mMagicEffects);

        state.mSummonedCreatureMap = mSummonedCreatures;
        state.mSummonGraveyard = mSummonGraveyard;

        state.mHasAiSettings = true;
        for (int i=0; i<4; ++i)
            mAiSettings[i].writeState (state.mAiSettings[i]);
    }

    void CreatureStats::readState (const ESM::CreatureStats& state)
    {
        for (int i=0; i<ESM::Attribute::Length; ++i)
            mAttributes[i].readState (state.mAttributes[i]);

        for (int i=0; i<3; ++i)
            mDynamic[i].readState (state.mDynamic[i]);

        mLastRestock = MWWorld::TimeStamp(state.mTradeTime);
        mGoldPool = state.mGoldPool;

        mDead = state.mDead;
        mDeathAnimationFinished = state.mDeathAnimationFinished;
        mDied = state.mDied;
        mMurdered = state.mMurdered;
        mTalkedTo = state.mTalkedTo;
        mAlarmed = state.mAlarmed;
        mAttacked = state.mAttacked;
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
        mRecalcMagicka = state.mRecalcDynamicStats;
        mDrawState = DrawState_(state.mDrawState);
        mLevel = state.mLevel;
        mActorId = state.mActorId;
        mDeathAnimation = state.mDeathAnimation;
        mTimeOfDeath = MWWorld::TimeStamp(state.mTimeOfDeath);
        //mHitAttemptActorId = state.mHitAttemptActorId;

        mSpells.readState(state.mSpells);
        mActiveSpells.readState(state.mActiveSpells);
        mAiSequence.readState(state.mAiSequence);
        mMagicEffects.readState(state.mMagicEffects);

        mSummonedCreatures = state.mSummonedCreatureMap;
        mSummonGraveyard = state.mSummonGraveyard;

        if (state.mHasAiSettings)
            for (int i=0; i<4; ++i)
                mAiSettings[i].readState(state.mAiSettings[i]);
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
        if (mActorId==-1)
            mActorId = sActorId++;

        return mActorId;
    }

    bool CreatureStats::matchesActorId (int id) const
    {
        return mActorId!=-1 && id==mActorId;
    }

    void CreatureStats::cleanup()
    {
        sActorId = 0;
    }

    void CreatureStats::writeActorIdCounter (ESM::ESMWriter& esm)
    {
        esm.startRecord(ESM::REC_ACTC);
        esm.writeHNT("COUN", sActorId);
        esm.endRecord(ESM::REC_ACTC);
    }

    void CreatureStats::readActorIdCounter (ESM::ESMReader& esm)
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

    std::map<CreatureStats::SummonKey, int>& CreatureStats::getSummonedCreatureMap()
    {
        return mSummonedCreatures;
    }

    std::vector<int>& CreatureStats::getSummonedCreatureGraveyard()
    {
        return mSummonGraveyard;
    }
}
