#ifndef GAME_MWMECHANICS_CREATURESTATS_H
#define GAME_MWMECHANICS_CREATURESTATS_H

#include <set>
#include <string>
#include <stdexcept>

#include "stat.hpp"
#include "magiceffects.hpp"
#include "spells.hpp"
#include "activespells.hpp"

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
    };

    // Inline const getters

    inline const Stat<int> &
    CreatureStats::getAttribute(int index) const {
        if (index < 0 || index > 7) {
            throw std::runtime_error("attribute index is out of range");
        }
        return mAttributes[index];
    }

    inline const DynamicStat<int> &
    CreatureStats::getHealth() const {
        return mDynamic[0];
    }

    inline const DynamicStat<int> &
    CreatureStats::getMagicka() const {
        return mDynamic[1];
    }

    inline const DynamicStat<int> &
    CreatureStats::getFatigue() const {
        return mDynamic[2];
    }

    inline const Spells &
    CreatureStats::getSpells() const {
        return mSpells;
    }

    inline const ActiveSpells & 
    CreatureStats::getActiveSpells() const {
        return mActiveSpells;
    }

    inline const MagicEffects & 
    CreatureStats::getMagicEffects() const {
        return mMagicEffects;
    }

    inline int
    CreatureStats::getLevel() const {
        return mLevel;
    }
   
    inline int
    CreatureStats::getHello() const {
        return mHello;
    }

    inline int
    CreatureStats::getFight() const {
        return mFight;
    }

    inline int
    CreatureStats::getFlee() const {
        return mFlee;
    }

    inline int
    CreatureStats::getAlarm() const {
        return mAlarm;
    }

    // Inline non-const getters

    inline Stat<int> &
    CreatureStats::getAttribute(int index) {
        if (index < 0 || index > 7) {
            throw std::runtime_error("attribute index is out of range");
        }
        return mAttributes[index];
    }

    inline DynamicStat<int> &
    CreatureStats::getHealth() {
        return mDynamic[0];
    }

    inline DynamicStat<int> &
    CreatureStats::getMagicka() {
        return mDynamic[1];
    }

    inline DynamicStat<int> &
    CreatureStats::getFatigue() {
        return mDynamic[2];
    }

    inline DynamicStat<int> &
    CreatureStats::getDynamic(int index) {
        if (index < 0 || index > 2) {
            throw std::runtime_error("dynamic stat index is out of range");
        }
        return mDynamic[index];
    }

    inline Spells & 
    CreatureStats::getSpells() {
        return mSpells;
    }

    inline void 
    CreatureStats::setSpells(const Spells &spells) {
        mSpells = spells;
    }

    inline ActiveSpells & 
    CreatureStats::getActiveSpells() {
        return mActiveSpells;
    }

    inline MagicEffects & 
    CreatureStats::getMagicEffects() {
        return mMagicEffects;
    }

    // Inline setters

    inline void
    CreatureStats::setAttribute(int index, const Stat<int> &value) {
        if (index < 0 || index > 7) {
            throw std::runtime_error("attribute index is out of range");
        }
        mAttributes[index] = value;
    }

    inline void
    CreatureStats::setHealth(const DynamicStat<int> &value) {
        mDynamic[0] = value;
    }

    inline void
    CreatureStats::setMagicka(const DynamicStat<int> &value) {
        mDynamic[1] = value;
    }

    inline void
    CreatureStats::setFatigue(const DynamicStat<int> &value) {
        mDynamic[2] = value;
    }

    inline void
    CreatureStats::setLevel(int level) {
        mLevel = level;
    }

    inline void 
    CreatureStats::setActiveSpells(const ActiveSpells &active) {
        mActiveSpells = active;
    }

    inline void 
    CreatureStats::setMagicEffects(const MagicEffects &effects) {
        mMagicEffects = effects;
    }

    inline void
    CreatureStats::setHello(int value) {
        mHello = value;
    }

    inline void
    CreatureStats::setFight(int value) {
        mFight = value;
    }

    inline void
    CreatureStats::setFlee(int value) {
        mFlee = value;
    }

    inline void
    CreatureStats::setAlarm(int value) {
        mAlarm = value;
    }
}

#endif
