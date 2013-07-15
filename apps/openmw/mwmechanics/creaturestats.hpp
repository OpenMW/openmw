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
        Stat<int> mAttributes[8];
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

    public:
        CreatureStats();

        const Stat<int> & getAttribute(int index) const;

        const DynamicStat<float> & getHealth() const;

        const DynamicStat<float> & getMagicka() const;

        const DynamicStat<float> & getFatigue() const;

        const DynamicStat<float> & getDynamic (int index) const;

        const Spells & getSpells() const;

        const ActiveSpells & getActiveSpells() const;

        const MagicEffects & getMagicEffects() const;

        const bool & getAttackingOrSpell() const;

        int getLevel() const;

        int getAiSetting (int index) const;
        ///< 0: hello, 1 fight, 2 flee, 3 alarm

        Stat<int> & getAttribute(int index);

        Spells & getSpells();

        ActiveSpells & getActiveSpells();

        MagicEffects & getMagicEffects();

        void setAttribute(int index, const Stat<int> &value);

        void setHealth(const DynamicStat<float> &value);

        void setMagicka(const DynamicStat<float> &value);

        void setFatigue(const DynamicStat<float> &value);

        void setDynamic (int index, const DynamicStat<float> &value);

        void setSpells(const Spells &spells);

        void setActiveSpells(const ActiveSpells &active);

        void setMagicEffects(const MagicEffects &effects);

        void setAttackingOrSpell(const bool &attackingOrSpell);

        void setLevel(int level);

        void setAiSetting (int index, int value);
        ///< 0: hello, 1 fight, 2 flee, 3 alarm

        const AiSequence& getAiSequence() const;

        AiSequence& getAiSequence();

        float getFatigueTerm() const;
        ///< Return effective fatigue

        // small hack to allow the fact that Health permanently increases by 10% of endurance on each level up
        void increaseLevelHealthBonus(float value);
        float getLevelHealthBonus() const;

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
    };
}

#endif
