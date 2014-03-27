#ifndef GAME_MWMECHANICS_CREATURESTATS_H
#define GAME_MWMECHANICS_CREATURESTATS_H

#include <set>
#include <string>
#include <stdexcept>

#include "stat.hpp"
#include "magiceffects.hpp"
#include "spells.hpp"
#include "activespells.hpp"
#include "aisequence.hpp"
#include "drawstate.hpp"

namespace ESM
{
    struct CreatureStats;
}

namespace MWMechanics
{
    /// \brief Common creature stats
    ///
    ///
    class CreatureStats
    {
        DrawState_ mDrawState;
        AttributeValue mAttributes[8];
        DynamicStat<float> mDynamic[3]; // health, magicka, fatigue
        Spells mSpells;
        ActiveSpells mActiveSpells;
        MagicEffects mMagicEffects;
        Stat<int> mAiSettings[4];
        AiSequence mAiSequence;
        bool mDead;
        bool mDied;
        int mFriendlyHits;
        bool mTalkedTo;
        bool mAlarmed;
        bool mAttacked;
        bool mHostile;
        bool mAttackingOrSpell;
        bool mKnockdown;
        bool mHitRecovery;
        bool mBlock;
        unsigned int mMovementFlags;
        float mAttackStrength; // Note only some creatures attack with weapons

        float mFallHeight;

        std::string mLastHitObject; // The last object to hit this actor

        // Do we need to recalculate stats derived from attributes or other factors?
        bool mRecalcDynamicStats;

        std::map<std::string, MWWorld::TimeStamp> mUsedPowers;

        MWWorld::TimeStamp mTradeTime; // Relates to NPC gold reset delay

        int mGoldPool; // the pool of merchant gold not in inventory

    protected:
        bool mIsWerewolf;
        AttributeValue mWerewolfAttributes[8];
        int mLevel;

    public:
        CreatureStats();

        DrawState_ getDrawState() const;
        void setDrawState(DrawState_ state);

        /// When attacking, stores how strong the attack should be (0 = weakest, 1 = strongest)
        float getAttackStrength() const;
        void setAttackStrength(float value);

        bool needToRecalcDynamicStats();

        void addToFallHeight(float height);

        /// Reset the fall height
        /// @return total fall height
        float land();

        bool canUsePower (const std::string& power) const;
        void usePower (const std::string& power);

        const AttributeValue & getAttribute(int index) const;

        const DynamicStat<float> & getHealth() const;

        const DynamicStat<float> & getMagicka() const;

        const DynamicStat<float> & getFatigue() const;

        const DynamicStat<float> & getDynamic (int index) const;

        const Spells & getSpells() const;

        const ActiveSpells & getActiveSpells() const;

        const MagicEffects & getMagicEffects() const;

        bool getAttackingOrSpell() const;

        int getLevel() const;

        Spells & getSpells();

        ActiveSpells & getActiveSpells();

        MagicEffects & getMagicEffects();

        void setAttribute(int index, const AttributeValue &value);
        // Shortcut to set only the base
        void setAttribute(int index, int base);

        void setHealth(const DynamicStat<float> &value);

        void setMagicka(const DynamicStat<float> &value);

        void setFatigue(const DynamicStat<float> &value);

        void setDynamic (int index, const DynamicStat<float> &value);

        void setSpells(const Spells &spells);

        void setActiveSpells(const ActiveSpells &active);

        void setMagicEffects(const MagicEffects &effects);

        void setAttackingOrSpell(bool attackingOrSpell);

        void setLevel(int level);

        enum AiSetting
        {
            AI_Hello = 0,
            AI_Fight = 1,
            AI_Flee = 2,
            AI_Alarm = 3
        };
        void setAiSetting (AiSetting index, Stat<int> value);
        void setAiSetting (AiSetting index, int base);
        Stat<int> getAiSetting (AiSetting index) const;

        const AiSequence& getAiSequence() const;

        AiSequence& getAiSequence();

        float getFatigueTerm() const;
        ///< Return effective fatigue

        bool isDead() const;

        bool hasDied() const;

        void clearHasDied();

        void resurrect();

        bool hasCommonDisease() const;

        bool hasBlightDisease() const;

        int getFriendlyHits() const;
        ///< Number of friendly hits received.

        void friendlyHit();
        ///< Increase number of friendly hits by one.

        bool hasTalkedToPlayer() const;
        ///< Has this creature talked with the player before?

        void talkedToPlayer();

        bool isAlarmed() const;

        void setAlarmed (bool alarmed);

        bool getAttacked() const;

        void setAttacked (bool attacked);

        bool isHostile() const;

        void setHostile (bool hostile);

        bool getCreatureTargetted() const;

        float getEvasion() const;

        void setKnockedDown(bool value);
        bool getKnockedDown() const;
        void setHitRecovery(bool value);
        bool getHitRecovery() const;
        void setBlock(bool value);
        bool getBlock() const;

        enum Flag
        {
            Flag_ForceRun = 1,
            Flag_ForceSneak = 2,
            Flag_Run = 4,
            Flag_Sneak = 8
        };
        enum Stance
        {
            Stance_Run,
            Stance_Sneak
        };

        bool getMovementFlag (Flag flag) const;
        void setMovementFlag (Flag flag, bool state);
        /// Like getMovementFlag, but also takes into account if the flag is Forced
        bool getStance (Stance flag) const;

        void setLastHitObject(const std::string &objectid);
        const std::string &getLastHitObject() const;

        // Note, this is just a cache to avoid checking the whole container store every frame TODO: Put it somewhere else?
        std::set<int> mBoundItems;
        // Same as above
        std::map<int, std::string> mSummonedCreatures;

        void writeState (ESM::CreatureStats& state) const;

        void readState (const ESM::CreatureStats& state);

        // Relates to NPC gold reset delay
        void setTradeTime(MWWorld::TimeStamp tradeTime);
        MWWorld::TimeStamp getTradeTime() const;

        void setGoldPool(int pool);
        int getGoldPool() const;
    };
}

#endif
