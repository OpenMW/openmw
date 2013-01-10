
#include "activespells.hpp"

#include <cstdlib>

#include <boost/algorithm/string.hpp>

#include <components/esm/loadalch.hpp>
#include <components/esm/loadspel.hpp>
#include <components/esm/loadingr.hpp>
#include <components/esm/loadmgef.hpp>
#include <components/esm/loadskil.hpp>

#include "../mwworld/esmstore.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"

#include "creaturestats.hpp"
#include "npcstats.hpp"

namespace MWMechanics
{
    void ActiveSpells::update() const
    {
        bool rebuild = false;

        MWWorld::TimeStamp now = MWBase::Environment::get().getWorld()->getTimeStamp();

        if (mLastUpdate!=now)
        {
            TContainer::iterator iter (mSpells.begin());
            while (iter!=mSpells.end())
                if (!timeToExpire (iter))
                {
                    mSpells.erase (iter++);
                    rebuild = true;
                }
                else
                    ++iter;

            mLastUpdate = now;
        }

        if (mSpellsChanged)
        {
            mSpellsChanged = false;
            rebuild = true;
        }

        if (rebuild)
            rebuildEffects();
    }

    void ActiveSpells::rebuildEffects() const
    {
        MWWorld::TimeStamp now = MWBase::Environment::get().getWorld()->getTimeStamp();
    
        mEffects = MagicEffects();

        for (TIterator iter (begin()); iter!=end(); ++iter)
        {
            std::pair<ESM::EffectList, bool> effects = getEffectList (iter->first);

            const MWWorld::TimeStamp& start = iter->second.first;
            float magnitude = iter->second.second;

            for (std::vector<ESM::ENAMstruct>::const_iterator iter (effects.first.mList.begin());
                iter!=effects.first.mList.end(); ++iter)
            {
                if (iter->mDuration)
                {
                    int duration = iter->mDuration;
                    
                    if (effects.second)
                        duration *= magnitude;
                    
                    MWWorld::TimeStamp end = start;
                    end += static_cast<double> (duration)*
                        MWBase::Environment::get().getWorld()->getTimeScaleFactor()/(60*60);

                    if (end>now)
                    {
                        EffectParam param;
                        
                        if (effects.second)
                        {
                            const ESM::MagicEffect *magicEffect =
                                MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find (
                                iter->mEffectID);                            
                                
                            if (iter->mDuration==0)
                            {
                                param.mMagnitude =
                                    static_cast<int> (magnitude / (0.1 * magicEffect->mData.mBaseCost));
                            }
                            else
                            {
                                param.mMagnitude =
                                    static_cast<int> (0.05*magnitude / (0.1 * magicEffect->mData.mBaseCost));
                            }
                        }
                        else
                            param.mMagnitude = static_cast<int> (
                                (iter->mMagnMax-iter->mMagnMin)*magnitude + iter->mMagnMin);
                                
                        mEffects.add (*iter, param);
                    }
                }
            }
        }    
    }

    std::pair<ESM::EffectList, bool> ActiveSpells::getEffectList (const std::string& id) const
    {
        if (const ESM::Spell *spell =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search (id))
            return std::make_pair (spell->mEffects, false);

        if (const ESM::Potion *potion =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Potion>().search (id))
            return std::make_pair (potion->mEffects, false);

        if (const ESM::Ingredient *ingredient =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Ingredient>().search (id))
        {
            const ESM::MagicEffect *magicEffect =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find (
                ingredient->mData.mEffectID[0]);
        
            ESM::ENAMstruct effect;
            effect.mEffectID = ingredient->mData.mEffectID[0];
            effect.mSkill = ingredient->mData.mSkills[0];
            effect.mAttribute = ingredient->mData.mAttributes[0];
            effect.mRange = 0;
            effect.mArea = 0;
            effect.mDuration = magicEffect->mData.mFlags & ESM::MagicEffect::NoDuration ? 0 : 1;
            effect.mMagnMin = 1;
            effect.mMagnMax = 1;
            
            std::pair<ESM::EffectList, bool> result;
            
            result.first.mList.push_back (effect);
            result.second = true;
            
            return result;
        }

        throw std::runtime_error ("ID " + id + " can not produce lasting effects");
    }

    ActiveSpells::ActiveSpells()
    : mSpellsChanged (false), mLastUpdate (MWBase::Environment::get().getWorld()->getTimeStamp())
    {}

    bool ActiveSpells::addSpell (const std::string& id, const MWWorld::Ptr& actor)
    {
        std::pair<ESM::EffectList, bool> effects = getEffectList (id);

        bool found = false;

        for (std::vector<ESM::ENAMstruct>::const_iterator iter (effects.first.mList.begin());
            iter!=effects.first.mList.end(); ++iter)
        {
            if (iter->mDuration)
            {
                found = true;
                break;
            }
        }

        if (!found)
            return false;

        TContainer::iterator iter = mSpells.find (id);

        float random = static_cast<float> (std::rand()) / RAND_MAX;

        if (effects.second)
        {
            // ingredient -> special treatment required.
            const CreatureStats& creatureStats = MWWorld::Class::get (actor).getCreatureStats (actor);
            const NpcStats& npcStats = MWWorld::Class::get (actor).getNpcStats (actor);
        
            float x =
                (npcStats.getSkill (ESM::Skill::Alchemy).getModified() +
                0.2 * creatureStats.getAttribute (1).getModified()
                + 0.1 * creatureStats.getAttribute (7).getModified())
                * creatureStats.getFatigueTerm();
            random *= 100;
            random = random / std::min (x, 100.0f);
            random *= 0.25 * x;
        }

        if (iter==mSpells.end())
            mSpells.insert (std::make_pair (id,
                std::make_pair (MWBase::Environment::get().getWorld()->getTimeStamp(), random)));
        else
            iter->second = std::make_pair (MWBase::Environment::get().getWorld()->getTimeStamp(), random);

        mSpellsChanged = true;

        return true;
    }

    void ActiveSpells::removeSpell (const std::string& id)
    {
        TContainer::iterator iter = mSpells.find (id);

        if (iter!=mSpells.end())
        {
            mSpells.erase (iter);
            mSpellsChanged = true;
        }
    }

    const MagicEffects& ActiveSpells::getMagicEffects() const
    {
        update();
        return mEffects;
    }

    ActiveSpells::TIterator ActiveSpells::begin() const
    {
        update();
        return mSpells.begin();
    }

    ActiveSpells::TIterator ActiveSpells::end() const
    {
        update();
        return mSpells.end();
    }

    double ActiveSpells::timeToExpire (const TIterator& iterator) const
    {
        std::pair<ESM::EffectList, bool> effects = getEffectList (iterator->first);

        int duration = 0;

        for (std::vector<ESM::ENAMstruct>::const_iterator iter (effects.first.mList.begin());
            iter!=effects.first.mList.end(); ++iter)
        {
            if (iter->mDuration > duration)
                duration = iter->mDuration;
        }

        if (effects.second)
            duration *= iterator->second.second;

        double scaledDuration = duration *
              MWBase::Environment::get().getWorld()->getTimeScaleFactor()/(60*60);

        double usedUp = MWBase::Environment::get().getWorld()->getTimeStamp()-iterator->second.first;

        if (usedUp>=scaledDuration)
            return 0;

        return scaledDuration-usedUp;
    }

    bool ActiveSpells::isSpellActive(std::string id) const
    {
        Misc::StringUtils::toLower(id);
        for (TContainer::iterator iter = mSpells.begin(); iter != mSpells.end(); ++iter)
        {
            std::string left = iter->first;
            Misc::StringUtils::toLower(left);

            if (iter->first == id)
                return true;
        }
        return false;
    }
}
