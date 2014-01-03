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

namespace MWMechanics
{
    /// \brief Common creature stats
    ///
    ///
    class CreatureStats
    {
        AttributeValue mAttributes[8];
        DynamicStat<float> mDynamic[3]; // health, magicka, fatigue
        int mLevel;
        Spells mSpells;
        ActiveSpells mActiveSpells;
        MagicEffects mMagicEffects;
        int mAiSettings[4];
        AiSequence mAiSequence;
        float mLevelHealthBonus;
        bool mDead;
        bool mDied;
        int mFriendlyHits;
        bool mTalkedTo;
        bool mAlarmed;
        bool mAttacked;
        bool mHostile;
        bool mAttackingOrSpell;//for the player, this is true if the left mouse button is pressed, false if not.

        float mFallHeight;

        int mAttackType;

        std::string mLastHitObject; // The last object to hit this actor

        // Do we need to recalculate stats derived from attributes or other factors?
        bool mRecalcDynamicStats;

        std::map<std::string, MWWorld::TimeStamp> mUsedPowers;

    protected:
        bool mIsWerewolf;
        AttributeValue mWerewolfAttributes[8];

    public:
        CreatureStats();

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

        int getAiSetting (int index) const;
        ///< 0: hello, 1 fight, 2 flee, 3 alarm

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

        enum AttackType
        {
            AT_Slash,
            AT_Thrust,
            AT_Chop
        };
        void setAttackType(int attackType) { mAttackType = attackType; }
        int getAttackType() { return mAttackType; }

        void setLevel(int level);

        void setAiSetting (int index, int value);
        ///< 0: hello, 1 fight, 2 flee, 3 alarm

        const AiSequence& getAiSequence() const;

        AiSequence& getAiSequence();

        float getFatigueTerm() const;
        ///< Return effective fatigue

        float getLevelHealthBonus() const;

        void levelUp();

        void updateHealth();
        ///< Calculate health based on endurance and strength.
        ///  Called at character creation and at level up.

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

        void setLastHitObject(const std::string &objectid);
        const std::string &getLastHitObject() const;

        // Note, this is just a cache to avoid checking the whole container store every frame TODO: Put it somewhere else?
        std::set<int> mBoundItems;
        // Same as above
        std::map<int, std::string> mSummonedCreatures;
    };
}

#endif
