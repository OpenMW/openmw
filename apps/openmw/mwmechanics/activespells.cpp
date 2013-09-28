
#include "activespells.hpp"

#include <cstdlib>

#include <components/esm/loadalch.hpp>
#include <components/esm/loadspel.hpp>

#include <components/esm_store/store.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

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
        {
            mEffects = MagicEffects();

            for (TIterator iter (begin()); iter!=end(); ++iter)
            {
                const ESM::EffectList& effects = getEffectList (iter->first);

                const MWWorld::TimeStamp& start = iter->second.first;
                float magnitude = iter->second.second;

                for (std::vector<ESM::ENAMstruct>::const_iterator iter (effects.list.begin());
                    iter!=effects.list.end(); ++iter)
                {
                    if (iter->duration)
                    {
                        MWWorld::TimeStamp end = start;
                        end += static_cast<double> (iter->duration)*
                            MWBase::Environment::get().getWorld()->getTimeScaleFactor()/(60*60);

                        if (end>now)
                        {
                            EffectParam param;
                            param.mMagnitude = static_cast<int> (
                                (iter->magnMax-iter->magnMin+1)*magnitude + iter->magnMin);
                            mEffects.add (*iter, param);
                        }
                    }
                }
            }
        }
    }

    const ESM::EffectList& ActiveSpells::getEffectList (const std::string& id) const
    {
        if (const ESM::Spell *spell =
            MWBase::Environment::get().getWorld()->getStore().spells.search (id))
            return spell->effects;

        if (const ESM::Potion *potion =
            MWBase::Environment::get().getWorld()->getStore().potions.search (id))
            return potion->effects;

        throw std::runtime_error ("ID " + id + " can not produce lasting effects");
    }

    ActiveSpells::ActiveSpells()
    : mSpellsChanged (false), mLastUpdate (MWBase::Environment::get().getWorld()->getTimeStamp())
    {}

    bool ActiveSpells::addSpell (const std::string& id)
    {
        const ESM::EffectList& effects = getEffectList (id);

        bool found = false;

        for (std::vector<ESM::ENAMstruct>::const_iterator iter (effects.list.begin());
            iter!=effects.list.end(); ++iter)
        {
            if (iter->duration)
            {
                found = true;
                break;
            }
        }

        if (!found)
            return false;

        TContainer::iterator iter = mSpells.find (id);

        float random = static_cast<float> (std::rand()) / RAND_MAX;

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
        const ESM::EffectList& effects = getEffectList (iterator->first);

        int duration = 0;

        for (std::vector<ESM::ENAMstruct>::const_iterator iter (effects.list.begin());
            iter!=effects.list.end(); ++iter)
        {
            if (iter->duration>duration)
                duration = iter->duration;
        }

        double scaledDuration = duration *
              MWBase::Environment::get().getWorld()->getTimeScaleFactor()/(60*60);

        double usedUp = MWBase::Environment::get().getWorld()->getTimeStamp()-iterator->second.first;

        if (usedUp>=scaledDuration)
            return 0;

        return scaledDuration-usedUp;
    }
}
