#include "creaturestats.hpp"

#include <algorithm>

#include <components/esm_store/store.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

namespace MWMechanics
{
    CreatureStats::CreatureStats()
    {}
    
    // Can't use all benefits of members initialization because of
    // lack of copy constructors
    CreatureStats::CreatureStats(const CreatureStats &orig)
      : mLevel(orig.mLevel), mHello(orig.mHello), mFight(orig.mFight),
      mFlee(orig.mFlee), mAlarm(orig.mAlarm)
    {
        for (int i = 0; i < 8; ++i) {
            mAttributes[i] = orig.mAttributes[i];
        }
        for (int i = 0; i < 3; ++i) {
            mDynamic[i] = orig.mDynamic[i];
        }
        mSpells = orig.mSpells;
        mActiveSpells = orig.mActiveSpells;
        mMagicEffects = orig.mMagicEffects;
    }

    CreatureStats::~CreatureStats()
    {}

    const CreatureStats &
    CreatureStats::operator=(const CreatureStats &orig)
    {
        for (int i = 0; i < 8; ++i) {
            mAttributes[i] = orig.mAttributes[i];
        }
        for (int i = 0; i < 3; ++i) {
            mDynamic[i] = orig.mDynamic[i];
        }
        mLevel = orig.mLevel;
        mSpells = orig.mSpells;
        mActiveSpells = orig.mActiveSpells;
        mMagicEffects = orig.mMagicEffects;
        mHello = orig.mHello;
        mFight = orig.mFight;
        mFlee = orig.mFlee;
        mAlarm = orig.mAlarm;

        return *this;
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

        const ESMS::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
         
        return store.gameSettings.find ("fFatigueBase")->getFloat()
            - store.gameSettings.find ("fFatigueMult")->getFloat() * (1-normalised);
    }
    
    const Stat<int> &CreatureStats::getAttribute(int index) const
    {
        if (index < 0 || index > 7) {
            throw std::runtime_error("attribute index is out of range");
        }
        return mAttributes[index];
    }

    const DynamicStat<int> &CreatureStats::getHealth() const
    {
        return mDynamic[0];
    }

    const DynamicStat<int> &CreatureStats::getMagicka() const
    {
        return mDynamic[1];
    }

    const DynamicStat<int> &CreatureStats::getFatigue() const
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
   
    int CreatureStats::getHello() const
    {
        return mHello;
    }

    int CreatureStats::getFight() const
    {
        return mFight;
    }

    int CreatureStats::getFlee() const
    {
        return mFlee;
    }

    int CreatureStats::getAlarm() const
    {
        return mAlarm;
    }

    Stat<int> &CreatureStats::getAttribute(int index)
    {
        if (index < 0 || index > 7) {
            throw std::runtime_error("attribute index is out of range");
        }
        return mAttributes[index];
    }

    DynamicStat<int> &CreatureStats::getHealth()
    {
        return mDynamic[0];
    }

    DynamicStat<int> &CreatureStats::getMagicka()
    {
        return mDynamic[1];
    }

    DynamicStat<int> &CreatureStats::getFatigue()
    {
        return mDynamic[2];
    }

    DynamicStat<int> &CreatureStats::getDynamic(int index)
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

    void CreatureStats::setAttribute(int index, const Stat<int> &value)
    {
        if (index < 0 || index > 7) {
            throw std::runtime_error("attribute index is out of range");
        }
        mAttributes[index] = value;
    }

    void CreatureStats::setHealth(const DynamicStat<int> &value)
    {
        mDynamic[0] = value;
    }

    void CreatureStats::setMagicka(const DynamicStat<int> &value)
    {
        mDynamic[1] = value;
    }

    void CreatureStats::setFatigue(const DynamicStat<int> &value)
    {
        mDynamic[2] = value;
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
        mMagicEffects = effects;
    }

    void CreatureStats::setHello(int value)
    {
        mHello = value;
    }

    void CreatureStats::setFight(int value)
    {
        mFight = value;
    }

    void CreatureStats::setFlee(int value)
    {
        mFlee = value;
    }

    void CreatureStats::setAlarm(int value)
    {
        mAlarm = value;
    }    
}
