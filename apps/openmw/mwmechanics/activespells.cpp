#include "activespells.hpp"

#include <components/misc/rng.hpp>
#include <components/misc/stringops.hpp>

#include <components/esm/loadmgef.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/esmstore.hpp"

namespace MWMechanics
{
    void ActiveSpells::update(float duration) const
    {
        bool rebuild = false;

        // Erase no longer active spells and effects
        if (duration > 0)
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
                    bool interrupt = false;
                    std::vector<ActiveEffect>& effects = iter->second.mEffects;
                    for (std::vector<ActiveEffect>::iterator effectIt = effects.begin(); effectIt != effects.end();)
                    {
                        if (effectIt->mTimeLeft <= 0)
                        {
                            rebuild = true;

                            // Note: it we expire a Corprus effect, we should remove the whole spell.
                            if (effectIt->mEffectId == ESM::MagicEffect::Corprus)
                            {
                                iter = mSpells.erase (iter);
                                interrupt = true;
                                break;
                            }

                            effectIt = effects.erase(effectIt);
                        }
                        else
                        {
                            effectIt->mTimeLeft -= duration;
                            ++effectIt;
                        }
                    }

                    if (!interrupt)
                        ++iter;
                }
            }
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
        mEffects = MagicEffects();

        for (TIterator iter (begin()); iter!=end(); ++iter)
        {
            const std::vector<ActiveEffect>& effects = iter->second.mEffects;

            for (std::vector<ActiveEffect>::const_iterator effectIt = effects.begin(); effectIt != effects.end(); ++effectIt)
            {
                if (effectIt->mTimeLeft > 0)
                    mEffects.add(MWMechanics::EffectKey(effectIt->mEffectId, effectIt->mArg), MWMechanics::EffectParam(effectIt->mMagnitude));
            }
        }
    }

    ActiveSpells::ActiveSpells()
        : mSpellsChanged (false)
    {}

    const MagicEffects& ActiveSpells::getMagicEffects() const
    {
        update(0.f);
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
            if (iter->mTimeLeft > duration)
                duration = iter->mTimeLeft;
        }

        if (duration < 0)
            return 0;

        return duration;
    }

    bool ActiveSpells::isSpellActive(const std::string& id) const
    {
        for (TContainer::iterator iter = mSpells.begin(); iter != mSpells.end(); ++iter)
        {
            if (Misc::StringUtils::ciEqual(iter->first, id))
                return true;
        }
        return false;
    }

    const ActiveSpells::TContainer& ActiveSpells::getActiveSpells() const
    {
        return mSpells;
    }

    void ActiveSpells::addSpell(const std::string &id, bool stack, const std::vector<ActiveEffect>& effects,
                                const std::string &displayName, int casterActorId)
    {
        TContainer::iterator it(mSpells.find(id));

        ActiveSpellParams params;
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
        for (TContainer::iterator spell = mSpells.begin(); spell != mSpells.end(); ++spell)
        {
            if (spell->first == id)
            {
                spell->second.mEffects.clear();
                mSpellsChanged = true;
            }
        }
    }

    void ActiveSpells::visitEffectSources(EffectSourceVisitor &visitor) const
    {
        for (TContainer::const_iterator it = begin(); it != end(); ++it)
        {
            for (std::vector<ActiveEffect>::const_iterator effectIt = it->second.mEffects.begin();
                 effectIt != it->second.mEffects.end(); ++effectIt)
            {
                std::string name = it->second.mDisplayName;

                float magnitude = effectIt->mMagnitude;
                if (magnitude)
                    visitor.visit(MWMechanics::EffectKey(effectIt->mEffectId, effectIt->mArg), effectIt->mEffectIndex, name, it->first, it->second.mCasterActorId, magnitude, effectIt->mTimeLeft, effectIt->mDuration);
            }
        }
    }

    void ActiveSpells::purgeAll(float chance, bool spellOnly)
    {
        for (TContainer::iterator it = mSpells.begin(); it != mSpells.end(); )
        {
            const std::string spellId = it->first;

            // if spellOnly is true, dispell only spells. Leave potions, enchanted items etc.
            if (spellOnly)
            {
                const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search(spellId);
                if (!spell || spell->mData.mType != ESM::Spell::ST_Spell)
                {
                    ++it;
                    continue;
                }
            }

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

    void ActiveSpells::purgeEffect(short effectId, const std::string& sourceId, int effectIndex)
    {
        for (TContainer::iterator it = mSpells.begin(); it != mSpells.end(); ++it)
        {
            for (std::vector<ActiveEffect>::iterator effectIt = it->second.mEffects.begin();
                 effectIt != it->second.mEffects.end();)
            {
                if (effectIt->mEffectId == effectId && it->first == sourceId && (effectIndex < 0 || effectIndex == effectIt->mEffectIndex))
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
                if (it->second.mCasterActorId == casterActorId)
                    effectIt = it->second.mEffects.erase(effectIt);
                else
                    ++effectIt;
            }
        }
        mSpellsChanged = true;
    }

    void ActiveSpells::purgeCorprusDisease()
    {
        for (TContainer::iterator iter = mSpells.begin(); iter!=mSpells.end();)
        {
            bool hasCorprusEffect = false;
            for (std::vector<ActiveEffect>::iterator effectIt = iter->second.mEffects.begin();
                 effectIt != iter->second.mEffects.end();++effectIt)
            {
                if (effectIt->mEffectId == ESM::MagicEffect::Corprus)
                {
                    hasCorprusEffect = true;
                    break;
                }
            }

            if (hasCorprusEffect)
            {
                mSpells.erase(iter++);
                mSpellsChanged = true;
            }
            else
                ++iter;
        }
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

            mSpells.insert (std::make_pair(it->first, params));
            mSpellsChanged = true;
        }
    }
}
