#include "creaturestats.hpp"

#include <algorithm>

#include "../mwworld/esmstore.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

namespace MWMechanics
{  
    CreatureStats::CreatureStats()
        : mLevel (0), mHello (0), mFight (0), mFlee (0), mAlarm (0), mLevelHealthBonus(0.f), mDead (false)
    {
    }

    void CreatureStats::increaseLevelHealthBonus (float value)
    {
        mLevelHealthBonus += value;
    }

    float CreatureStats::getLevelHealthBonus () const
    {
        return mLevelHealthBonus;
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
    
    const Stat<int> &CreatureStats::getAttribute(int index) const
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

    void CreatureStats::setAttribute(int index, const Stat<int> &value)
    {
        if (index < 0 || index > 7) {
            throw std::runtime_error("attribute index is out of range");
        }
        mAttributes[index] = value;
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
            mDead = true;
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
    
    bool CreatureStats::isDead() const
    {
        return mDead;
    }
    
    void CreatureStats::resurrect()
    {
        if (mDead)
        {
            if (mDynamic[0].getCurrent()<1)
                mDynamic[0].setCurrent (1);
                
            if (mDynamic[0].getCurrent()>=1)
                mDead = false;
        }
    }
}
