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
        DynamicStat<int> mDynamic[3]; // health, magicka, fatigue
        int mLevel;
        Spells mSpells;
        ActiveSpells mActiveSpells;
        MagicEffects mMagicEffects;
        int mHello;
        int mFight;
        int mFlee;
        int mAlarm;
        AiSequence mAiSequence;

    public:
        CreatureStats();
        CreatureStats(const CreatureStats &);
        virtual ~CreatureStats();

        const CreatureStats & operator=(const CreatureStats &);

        const Stat<int> & getAttribute(int index) const;

        const DynamicStat<int> & getHealth() const;

        const DynamicStat<int> & getMagicka() const;

        const DynamicStat<int> & getFatigue() const;

        const Spells & getSpells() const;

        const ActiveSpells & getActiveSpells() const;

        const MagicEffects & getMagicEffects() const;

        int getLevel() const;

        int getHello() const;

        int getFight() const;

        int getFlee() const;

        int getAlarm() const;


        Stat<int> & getAttribute(int index);

        DynamicStat<int> & getHealth();

        DynamicStat<int> & getMagicka();

        DynamicStat<int> & getFatigue();

        DynamicStat<int> & getDynamic(int index);

        Spells & getSpells();

        ActiveSpells & getActiveSpells();

        MagicEffects & getMagicEffects();


        void setAttribute(int index, const Stat<int> &value);

        void setHealth(const DynamicStat<int> &value);

        void setMagicka(const DynamicStat<int> &value);

        void setFatigue(const DynamicStat<int> &value);

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
    };
}

#endif
