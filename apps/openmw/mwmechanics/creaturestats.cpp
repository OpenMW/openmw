#include "creaturestats.hpp"

#include <algorithm>

#include <components/esm/creaturestats.hpp>

#include "../mwworld/esmstore.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

namespace MWMechanics
{
    int CreatureStats::sActorId = 0;

    CreatureStats::CreatureStats()
        : mLevel (0), mDead (false), mDied (false), mFriendlyHits (0),
          mTalkedTo (false), mAlarmed (false),
          mAttacked (false), mHostile (false),
          mAttackingOrSpell(false),
          mIsWerewolf(false),
          mFallHeight(0), mRecalcDynamicStats(false), mKnockdown(false), mKnockdownOneFrame(false),
          mKnockdownOverOneFrame(false), mHitRecovery(false), mBlock(false),
          mMovementFlags(0), mDrawState (DrawState_Nothing), mAttackStrength(0.f),
          mTradeTime(0,0), mGoldPool(0), mActorId(-1)
    {
        for (int i=0; i<4; ++i)
            mAiSettings[i] = 0;
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
        int max = getFatigue().getModified();
        int current = getFatigue().getCurrent();

        float normalised = max==0 ? 1 : std::max (0.0f, static_cast<float> (current)/max);

        const MWWorld::Store<ESM::GameSetting> &gmst =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        return gmst.find ("fFatigueBase")->getFloat()
            - gmst.find ("fFatigueMult")->getFloat() * (1-normalised);
    }

    const AttributeValue &CreatureStats::getAttribute(int index) const
    {
        if (index < 0 || index > 7) {
            throw std::runtime_error("attribute index is out of range");
        }
        return (!mIsWerewolf ? mAttributes[index] : mWerewolfAttributes[index]);
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

    bool CreatureStats::getAttackingOrSpell() const
    {
        return mAttackingOrSpell;
    }

    int CreatureStats::getLevel() const
    {
        return mLevel;
    }

    Stat<int> CreatureStats::getAiSetting (AiSetting index) const
    {
        assert (index>=0 && index<4);
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

    void CreatureStats::setSpells(const Spells &spells)
    {
        mSpells = spells;
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

        const AttributeValue& currentValue = !mIsWerewolf ? mAttributes[index] : mWerewolfAttributes[index];

        if (value != currentValue)
        {
            if (index != ESM::Attribute::Luck
                    && index != ESM::Attribute::Personality
                    && index != ESM::Attribute::Speed)
                mRecalcDynamicStats = true;
        }

        if(!mIsWerewolf)
            mAttributes[index] = value;
        else
            mWerewolfAttributes[index] = value;
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
                mDied = true;

            mDead = true;
        }
    }

    void CreatureStats::setLevel(int level)
    {
        mLevel = level;
    }

    void CreatureStats::setActiveSpells(const ActiveSpells &active)
    {
        mActiveSpells = active;
    }

    void CreatureStats::setMagicEffects(const MagicEffects &effects)
    {
        if (effects.get(ESM::MagicEffect::FortifyMaximumMagicka).mMagnitude
                != mMagicEffects.get(ESM::MagicEffect::FortifyMaximumMagicka).mMagnitude)
            mRecalcDynamicStats = true;

        mMagicEffects = effects;
    }

    void CreatureStats::setAttackingOrSpell(bool attackingOrSpell)
    {
        mAttackingOrSpell = attackingOrSpell;
    }

    void CreatureStats::setAiSetting (AiSetting index, Stat<int> value)
    {
        assert (index>=0 && index<4);
        mAiSettings[index] = value;
    }

    void CreatureStats::setAiSetting (AiSetting index, int base)
    {
        Stat<int> stat = getAiSetting(index);
        stat.setBase(base);
        setAiSetting(index, stat);
    }

    bool CreatureStats::isDead() const
    {
        return mDead;
    }

    bool CreatureStats::hasDied() const
    {
        return mDied;
    }

    void CreatureStats::clearHasDied()
    {
        mDied = false;
    }

    void CreatureStats::resurrect()
    {
        if (mDead)
        {
            if (mDynamic[0].getCurrent()<1)
            {
                mDynamic[0].setModified(mDynamic[0].getModified(), 1);
                mDynamic[0].setCurrent(1);
            }
            if (mDynamic[0].getCurrent()>=1)
                mDead = false;
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

    bool CreatureStats::isHostile() const
    {
        return mHostile;
    }

    void CreatureStats::setHostile (bool hostile)
    {
        mHostile = hostile;
    }

    bool CreatureStats::getCreatureTargetted() const
    {
        std::string target;
        if (mAiSequence.getCombatTarget(target))
        {
            MWWorld::Ptr targetPtr;
            targetPtr = MWBase::Environment::get().getWorld()->getPtr(target, true);
            return targetPtr.getTypeName() == typeid(ESM::Creature).name();
        }
        return false;
    }

    float CreatureStats::getEvasion() const
    {
        float evasion = (getAttribute(ESM::Attribute::Agility).getModified() / 5.0f) +
                        (getAttribute(ESM::Attribute::Luck).getModified() / 10.0f);
        evasion *= getFatigueTerm();
        evasion += mMagicEffects.get(ESM::MagicEffect::Sanctuary).mMagnitude;

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

    void CreatureStats::addToFallHeight(float height)
    {
        mFallHeight += height;
    }

    float CreatureStats::land()
    {
        float height = mFallHeight;
        mFallHeight = 0;
        return height;
    }

    bool CreatureStats::needToRecalcDynamicStats()
    {
         if (mRecalcDynamicStats)
         {
             mRecalcDynamicStats = false;
             return true;
         }
         return false;
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
        return mMovementFlags & flag;
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

    float CreatureStats::getAttackStrength() const
    {
        return mAttackStrength;
    }

    void CreatureStats::setAttackStrength(float value)
    {
        mAttackStrength = value;
    }

    void CreatureStats::writeState (ESM::CreatureStats& state) const
    {
        for (int i=0; i<ESM::Attribute::Length; ++i)
            mAttributes[i].writeState (state.mAttributes[i]);

        for (int i=0; i<3; ++i)
            mDynamic[i].writeState (state.mDynamic[i]);

        state.mTradeTime = mTradeTime.toEsm();
        state.mGoldPool = mGoldPool;

        state.mDead = mDead;
        state.mDied = mDied;
        state.mFriendlyHits = mFriendlyHits;
        state.mTalkedTo = mTalkedTo;
        state.mAlarmed = mAlarmed;
        state.mAttacked = mAttacked;
        state.mHostile = mHostile;
        state.mAttackingOrSpell = mAttackingOrSpell;
        // TODO: rewrite. does this really need 3 separate bools?
        state.mKnockdown = mKnockdown;
        state.mKnockdownOneFrame = mKnockdownOneFrame;
        state.mKnockdownOverOneFrame = mKnockdownOverOneFrame;
        state.mHitRecovery = mHitRecovery;
        state.mBlock = mBlock;
        state.mMovementFlags = mMovementFlags;
        state.mAttackStrength = mAttackStrength;
        state.mFallHeight = mFallHeight; // TODO: vertical velocity (move from PhysicActor to CreatureStats?)
        state.mLastHitObject = mLastHitObject;
        state.mRecalcDynamicStats = mRecalcDynamicStats;
        state.mDrawState = mDrawState;
        state.mLevel = mLevel;
        state.mActorId = mActorId;

        mSpells.writeState(state.mSpells);
        mActiveSpells.writeState(state.mActiveSpells);
    }

    void CreatureStats::readState (const ESM::CreatureStats& state)
    {
        for (int i=0; i<ESM::Attribute::Length; ++i)
            mAttributes[i].readState (state.mAttributes[i]);

        for (int i=0; i<3; ++i)
            mDynamic[i].readState (state.mDynamic[i]);

        mTradeTime = MWWorld::TimeStamp(state.mTradeTime);
        mGoldPool = state.mGoldPool;
        mFallHeight = state.mFallHeight;

        mDead = state.mDead;
        mDied = state.mDied;
        mFriendlyHits = state.mFriendlyHits;
        mTalkedTo = state.mTalkedTo;
        mAlarmed = state.mAlarmed;
        mAttacked = state.mAttacked;
        mHostile = state.mHostile;
        mAttackingOrSpell = state.mAttackingOrSpell;
        // TODO: rewrite. does this really need 3 separate bools?
        mKnockdown = state.mKnockdown;
        mKnockdownOneFrame = state.mKnockdownOneFrame;
        mKnockdownOverOneFrame = state.mKnockdownOverOneFrame;
        mHitRecovery = state.mHitRecovery;
        mBlock = state.mBlock;
        mMovementFlags = state.mMovementFlags;
        mAttackStrength = state.mAttackStrength;
        mFallHeight = state.mFallHeight;
        mLastHitObject = state.mLastHitObject;
        mRecalcDynamicStats = state.mRecalcDynamicStats;
        mDrawState = DrawState_(state.mDrawState);
        mLevel = state.mLevel;
        mActorId = state.mActorId;

        mSpells.readState(state.mSpells);
        mActiveSpells.readState(state.mActiveSpells);
    }

    // Relates to NPC gold reset delay
    void CreatureStats::setTradeTime(MWWorld::TimeStamp tradeTime)
    {
        mTradeTime = tradeTime;
    }

    MWWorld::TimeStamp CreatureStats::getTradeTime() const
    {
        return mTradeTime;
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
}
