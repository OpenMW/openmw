#include "activespells.hpp"

#include <components/misc/rng.hpp>

#include <components/misc/stringops.hpp>

#include <components/esm/loadmgef.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/esmstore.hpp"

namespace MWMechanics
{
    void ActiveSpells::update() const
    {
        bool rebuild = false;

        MWWorld::TimeStamp now = MWBase::Environment::get().getWorld()->getTimeStamp();

        // Erase no longer active spells and effects
        if (mLastUpdate!=now)
        {
            TContainer::iterator iter (mSpells.begin());
            while (iter!=mSpells.end())
            {
                if (!timeToExpire (iter))
                {
                    mSpells.erase (iter++);
                    rebuild = true;
                }
                else
                {
                    std::vector<ActiveEffect>& effects = iter->second.mEffects;
                    for (std::vector<ActiveEffect>::iterator effectIt = effects.begin(); effectIt != effects.end();)
                    {
                        MWWorld::TimeStamp start = iter->second.mTimeStamp;
                        MWWorld::TimeStamp end = start + static_cast<double>(effectIt->mDuration)*MWBase::Environment::get().getWorld()->getTimeScaleFactor()/(60*60);
                        if (end <= now)
                        {
                            effectIt = effects.erase(effectIt);
                            rebuild = true;
                        }
                        else
                            ++effectIt;
                    }
                    ++iter;
                }
            }

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
            const MWWorld::TimeStamp& start = iter->second.mTimeStamp;

            const std::vector<ActiveEffect>& effects = iter->second.mEffects;

            for (std::vector<ActiveEffect>::const_iterator effectIt = effects.begin(); effectIt != effects.end(); ++effectIt)
            {
                double duration = effectIt->mDuration;
                MWWorld::TimeStamp end = start;
                end += duration * 
                    MWBase::Environment::get().getWorld()->getTimeScaleFactor()/(60*60);

                if (end>now)
                    mEffects.add(MWMechanics::EffectKey(effectIt->mEffectId, effectIt->mArg), MWMechanics::EffectParam(effectIt->mMagnitude));
            }
        }
    }

    ActiveSpells::ActiveSpells()
        : mSpellsChanged (false)
        , mLastUpdate (MWBase::Environment::get().getWorld()->getTimeStamp())
    {}

    const MagicEffects& ActiveSpells::getMagicEffects() const
    {
        update();
        return mEffects;
    }

    ActiveSpells::TIterator ActiveSpells::begin() const
    {
        return mSpells.begin();
    }

    ActiveSpells::TIterator ActiveSpells::end() const
    {
        return mSpells.end();
    }

    double ActiveSpells::timeToExpire (const TIterator& iterator) const
    {
        const std::vector<ActiveEffect>& effects = iterator->second.mEffects;

        float duration = 0;

        for (std::vector<ActiveEffect>::const_iterator iter (effects.begin());
            iter!=effects.end(); ++iter)
        {
            if (iter->mDuration > duration)
                duration = iter->mDuration;
        }

        double scaledDuration = duration *
              MWBase::Environment::get().getWorld()->getTimeScaleFactor()/(60*60);

        double usedUp = MWBase::Environment::get().getWorld()->getTimeStamp() - iterator->second.mTimeStamp;

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

    const ActiveSpells::TContainer& ActiveSpells::getActiveSpells() const
    {
        return mSpells;
    }

    void ActiveSpells::addSpell(const std::string &id, bool stack, std::vector<ActiveEffect> effects,
                                const std::string &displayName, int casterActorId)
    {
        TContainer::iterator it(mSpells.find(id));

        ActiveSpellParams params;
        params.mTimeStamp = MWBase::Environment::get().getWorld()->getTimeStamp();
        params.mEffects = effects;
        params.mDisplayName = displayName;
        params.mCasterActorId = casterActorId;

        if (it == end() || stack)
        {
            mSpells.insert(std::make_pair(id, params));
        }
        else
        {
            // addSpell() is called with effects for a range.
            // but a spell may have effects with different ranges (e.g. Touch & Target)
            // so, if we see new effects for same spell assume additional 
            // spell effects and add to existing effects of spell
            mergeEffects(params.mEffects, it->second.mEffects);
            it->second = params;
        }

        mSpellsChanged = true;
    }

    void ActiveSpells::mergeEffects(std::vector<ActiveEffect>& addTo, const std::vector<ActiveEffect>& from)
    {
        for (std::vector<ActiveEffect>::const_iterator effect(from.begin()); effect != from.end(); ++effect)
        {
            // if effect is not in addTo, add it
            bool missing = true;
            for (std::vector<ActiveEffect>::const_iterator iter(addTo.begin()); iter != addTo.end(); ++iter)
            {
                if ((effect->mEffectId == iter->mEffectId) && (effect->mArg == iter->mArg))
                {
                    missing = false;
                    break;
                }
            }
            if (missing)
            {
                addTo.push_back(*effect);
            }
        }
    }

    void ActiveSpells::removeEffects(const std::string &id)
    {
        mSpells.erase(Misc::StringUtils::lowerCase(id));
        mSpellsChanged = true;
    }

    void ActiveSpells::visitEffectSources(EffectSourceVisitor &visitor) const
    {
        for (TContainer::const_iterator it = begin(); it != end(); ++it)
        {
            float timeScale = MWBase::Environment::get().getWorld()->getTimeScaleFactor();

            for (std::vector<ActiveEffect>::const_iterator effectIt = it->second.mEffects.begin();
                 effectIt != it->second.mEffects.end(); ++effectIt)
            {
                std::string name = it->second.mDisplayName;

                float remainingTime = effectIt->mDuration +
                        static_cast<float>(it->second.mTimeStamp - MWBase::Environment::get().getWorld()->getTimeStamp())*3600/timeScale;
                float magnitude = effectIt->mMagnitude;

                if (magnitude)
                    visitor.visit(MWMechanics::EffectKey(effectIt->mEffectId, effectIt->mArg), name, it->first, it->second.mCasterActorId, magnitude, remainingTime, effectIt->mDuration);
            }
        }
    }

    void ActiveSpells::purgeAll(float chance)
    {
        for (TContainer::iterator it = mSpells.begin(); it != mSpells.end(); )
        {
            if (Misc::Rng::roll0to99() < chance)
                mSpells.erase(it++);
            else
                ++it;
        }
        mSpellsChanged = true;
    }

    void ActiveSpells::purgeEffect(short effectId)
    {
        for (TContainer::iterator it = mSpells.begin(); it != mSpells.end(); ++it)
        {
            for (std::vector<ActiveEffect>::iterator effectIt = it->second.mEffects.begin();
                 effectIt != it->second.mEffects.end();)
            {
                if (effectIt->mEffectId == effectId)
                    effectIt = it->second.mEffects.erase(effectIt);
                else
                    ++effectIt;
            }
        }
        mSpellsChanged = true;
    }

    void ActiveSpells::purgeEffect(short effectId, const std::string& sourceId)
    {
        for (TContainer::iterator it = mSpells.begin(); it != mSpells.end(); ++it)
        {
            for (std::vector<ActiveEffect>::iterator effectIt = it->second.mEffects.begin();
                 effectIt != it->second.mEffects.end();)
            {
                if (effectIt->mEffectId == effectId && it->first == sourceId)
                    effectIt = it->second.mEffects.erase(effectIt);
                else
                    ++effectIt;
            }
        }
        mSpellsChanged = true;
    }

    void ActiveSpells::purge(int casterActorId)
    {
        for (TContainer::iterator it = mSpells.begin(); it != mSpells.end(); ++it)
        {
            for (std::vector<ActiveEffect>::iterator effectIt = it->second.mEffects.begin();
                 effectIt != it->second.mEffects.end();)
            {
                const ESM::MagicEffect* effect = MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find(effectIt->mEffectId);
                if (effect->mData.mFlags & ESM::MagicEffect::CasterLinked
                        && it->second.mCasterActorId == casterActorId)
                    effectIt = it->second.mEffects.erase(effectIt);
                else
                    ++effectIt;
            }
        }
        mSpellsChanged = true;
    }

    void ActiveSpells::clear()
    {
        mSpells.clear();
        mSpellsChanged = true;
    }

    void ActiveSpells::writeState(ESM::ActiveSpells &state) const
    {
        for (TContainer::const_iterator it = mSpells.begin(); it != mSpells.end(); ++it)
        {
            // Stupid copying of almost identical structures. ESM::TimeStamp <-> MWWorld::TimeStamp
            ESM::ActiveSpells::ActiveSpellParams params;
            params.mEffects = it->second.mEffects;
            params.mCasterActorId = it->second.mCasterActorId;
            params.mDisplayName = it->second.mDisplayName;
            params.mTimeStamp = it->second.mTimeStamp.toEsm();

            state.mSpells.insert (std::make_pair(it->first, params));
        }
    }

    void ActiveSpells::readState(const ESM::ActiveSpells &state)
    {
        for (ESM::ActiveSpells::TContainer::const_iterator it = state.mSpells.begin(); it != state.mSpells.end(); ++it)
        {
            // Stupid copying of almost identical structures. ESM::TimeStamp <-> MWWorld::TimeStamp
            ActiveSpellParams params;
            params.mEffects = it->second.mEffects;
            params.mCasterActorId = it->second.mCasterActorId;
            params.mDisplayName = it->second.mDisplayName;
            params.mTimeStamp = MWWorld::TimeStamp(it->second.mTimeStamp);

            mSpells.insert (std::make_pair(it->first, params));
            mSpellsChanged = true;
        }
    }
}
