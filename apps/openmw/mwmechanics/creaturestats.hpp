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
        int mHello;
        int mFight;
        int mFlee;
        int mAlarm;
        AiSequence mAiSequence;

        float mLevelHealthBonus;

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

        int getLevel() const;

        int getHello() const;

        int getFight() const;

        int getFlee() const;

        int getAlarm() const;

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

        void setLevel(int level);

        void setHello(int value);

        void setFight(int value);

        void setFlee(int value);

        void setAlarm(int value);
        
        const AiSequence& getAiSequence() const;
        
        AiSequence& getAiSequence();
   
        float getFatigueTerm() const;
        ///< Return effective fatigue

        // small hack to allow the fact that Health permanently increases by 10% of endurance on each level up
        void increaseLevelHealthBonus(float value);
        float getLevelHealthBonus() const;
    };
}

#endif
