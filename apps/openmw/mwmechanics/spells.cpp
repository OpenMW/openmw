#include "spells.hpp"

#include <cstdlib>

#include <components/esm/loadspel.hpp>
#include <components/esm/spellstate.hpp>
#include <components/misc/rng.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/esmstore.hpp"

#include "magiceffects.hpp"

namespace MWMechanics
{
    Spells::TIterator Spells::begin() const
    {
        return mSpells.begin();
    }

    Spells::TIterator Spells::end() const
    {
        return mSpells.end();
    }

    const ESM::Spell* Spells::getSpell(const std::string& id) const
    {
        return MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find(id);
    }

    bool Spells::hasSpell(const std::string &spell) const
    {
        return hasSpell(getSpell(spell));
    }

    bool Spells::hasSpell(const ESM::Spell *spell) const
    {
        return mSpells.find(spell) != mSpells.end();
    }

    void Spells::add (const ESM::Spell* spell)
    {
        if (mSpells.find (spell)==mSpells.end())
        {
            std::map<const int, float> random;

            // Determine the random magnitudes (unless this is a castable spell, in which case
            // they will be determined when the spell is cast)
            if (spell->mData.mType != ESM::Spell::ST_Power && spell->mData.mType != ESM::Spell::ST_Spell)
            {
                for (unsigned int i=0; i<spell->mEffects.mList.size();++i)
                {
                    if (spell->mEffects.mList[i].mMagnMin != spell->mEffects.mList[i].mMagnMax)
                        random[i] = Misc::Rng::rollClosedProbability();
                }
            }

            if (hasCorprusEffect(spell))
            {
                CorprusStats corprus;
                corprus.mWorsenings = 0;
                corprus.mNextWorsening = MWBase::Environment::get().getWorld()->getTimeStamp() + CorprusStats::sWorseningPeriod;

                mCorprusSpells[spell] = corprus;
            }

            mSpells.insert (std::make_pair (spell, random));
        }
    }

    void Spells::add (const std::string& spellId)
    {
        add(getSpell(spellId));
    }

    void Spells::remove (const std::string& spellId)
    {
        const ESM::Spell* spell = getSpell(spellId);
        TContainer::iterator iter = mSpells.find (spell);

        std::map<SpellKey, CorprusStats>::iterator corprusIt = mCorprusSpells.find(spell);

        // if it's corprus, remove negative and keep positive effects
        if (corprusIt != mCorprusSpells.end())
        {
            worsenCorprus(spell);
            if (mPermanentSpellEffects.find(spell) != mPermanentSpellEffects.end())
            {
                MagicEffects & effects = mPermanentSpellEffects[spell];
                for (MagicEffects::Collection::const_iterator effectIt = effects.begin(); effectIt != effects.end();)
                {
                    const ESM::MagicEffect * magicEffect = MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find(effectIt->first.mId);
                    if (magicEffect->mData.mFlags & ESM::MagicEffect::Harmful)
                        effects.remove((effectIt++)->first);
                    else
                        ++effectIt;
                }
            }
            mCorprusSpells.erase(corprusIt);
        }

        if (iter!=mSpells.end())
            mSpells.erase (iter);

        if (spellId==mSelectedSpell)
            mSelectedSpell.clear();
    }

    MagicEffects Spells::getMagicEffects() const
    {
        // TODO: These are recalculated every frame, no need to do that

        MagicEffects effects;

        for (TIterator iter = mSpells.begin(); iter!=mSpells.end(); ++iter)
        {
            const ESM::Spell *spell = iter->first;

            if (spell->mData.mType==ESM::Spell::ST_Ability || spell->mData.mType==ESM::Spell::ST_Blight ||
                spell->mData.mType==ESM::Spell::ST_Disease || spell->mData.mType==ESM::Spell::ST_Curse)
            {
                int i=0;
                for (std::vector<ESM::ENAMstruct>::const_iterator it = spell->mEffects.mList.begin(); it != spell->mEffects.mList.end(); ++it)
                {
                    float random = 1.f;
                    if (iter->second.find(i) != iter->second.end())
                        random = iter->second.at(i);

                    effects.add (*it, it->mMagnMin + (it->mMagnMax - it->mMagnMin) * random);
                    ++i;
                }
            }
        }

        for (std::map<SpellKey, MagicEffects>::const_iterator it = mPermanentSpellEffects.begin(); it != mPermanentSpellEffects.end(); ++it)
        {
            effects += it->second;
        }

        return effects;
    }

    void Spells::clear()
    {
        mSpells.clear();
    }

    void Spells::setSelectedSpell (const std::string& spellId)
    {
        mSelectedSpell = spellId;
    }

    const std::string Spells::getSelectedSpell() const
    {
        return mSelectedSpell;
    }

    bool Spells::isSpellActive(const std::string &id) const
    {
        TContainer::const_iterator found = mSpells.find(getSpell(id));
        if (found != mSpells.end())
        {
            const ESM::Spell *spell = found->first;

            return (spell->mData.mType==ESM::Spell::ST_Ability || spell->mData.mType==ESM::Spell::ST_Blight ||
                spell->mData.mType==ESM::Spell::ST_Disease || spell->mData.mType==ESM::Spell::ST_Curse);
        }
        return false;
    }

    bool Spells::hasCommonDisease() const
    {
        for (TIterator iter = mSpells.begin(); iter!=mSpells.end(); ++iter)
        {
            const ESM::Spell *spell = iter->first;
            if (spell->mData.mType == ESM::Spell::ST_Disease)
                return true;
        }

        return false;
    }

    bool Spells::hasBlightDisease() const
    {
        for (TIterator iter = mSpells.begin(); iter!=mSpells.end(); ++iter)
        {
            const ESM::Spell *spell = iter->first;
            if (spell->mData.mType == ESM::Spell::ST_Blight)
                return true;
        }

        return false;
    }

    void Spells::purgeCommonDisease()
    {
        for (TContainer::iterator iter = mSpells.begin(); iter!=mSpells.end();)
        {
            const ESM::Spell *spell = iter->first;
            if (spell->mData.mType == ESM::Spell::ST_Disease)
                mSpells.erase(iter++);
            else
                ++iter;
        }
    }

    void Spells::purgeBlightDisease()
    {
        for (TContainer::iterator iter = mSpells.begin(); iter!=mSpells.end();)
        {
            const ESM::Spell *spell = iter->first;
            if (spell->mData.mType == ESM::Spell::ST_Blight && !hasCorprusEffect(spell))
                mSpells.erase(iter++);
            else
                ++iter;
        }
    }

    void Spells::purgeCorprusDisease()
    {
        for (TContainer::iterator iter = mSpells.begin(); iter!=mSpells.end();)
        {
            const ESM::Spell *spell = iter->first;
            if (hasCorprusEffect(spell))
                mSpells.erase(iter++);
            else
                ++iter;
        }
    }

    void Spells::purgeCurses()
    {
        for (TContainer::iterator iter = mSpells.begin(); iter!=mSpells.end();)
        {
            const ESM::Spell *spell = iter->first;
            if (spell->mData.mType == ESM::Spell::ST_Curse)
                mSpells.erase(iter++);
            else
                ++iter;
        }
    }

    void Spells::visitEffectSources(EffectSourceVisitor &visitor) const
    {
        for (TIterator it = begin(); it != end(); ++it)
        {
            const ESM::Spell* spell = it->first;

            // these are the spell types that are permanently in effect
            if (!(spell->mData.mType == ESM::Spell::ST_Ability)
                    && !(spell->mData.mType == ESM::Spell::ST_Disease)
                    && !(spell->mData.mType == ESM::Spell::ST_Curse)
                    && !(spell->mData.mType == ESM::Spell::ST_Blight))
                continue;
            const ESM::EffectList& list = spell->mEffects;
            int i=0;
            for (std::vector<ESM::ENAMstruct>::const_iterator effectIt = list.mList.begin();
                 effectIt != list.mList.end(); ++effectIt, ++i)
            {
                float random = 1.f;
                if (it->second.find(i) != it->second.end())
                    random = it->second.at(i);

                float magnitude = effectIt->mMagnMin + (effectIt->mMagnMax - effectIt->mMagnMin) * random;
                visitor.visit(MWMechanics::EffectKey(*effectIt), spell->mName, spell->mId, -1, magnitude);
            }
        }
    }

    void Spells::worsenCorprus(const ESM::Spell* spell)
    {
        mCorprusSpells[spell].mNextWorsening = MWBase::Environment::get().getWorld()->getTimeStamp() + CorprusStats::sWorseningPeriod;
        mCorprusSpells[spell].mWorsenings++;

        // update worsened effects
        mPermanentSpellEffects[spell] = MagicEffects();
        int i=0;
        for (std::vector<ESM::ENAMstruct>::const_iterator effectIt = spell->mEffects.mList.begin(); effectIt != spell->mEffects.mList.end(); ++effectIt, ++i)
        {
            const ESM::MagicEffect * magicEffect = MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find(effectIt->mEffectID);
            if ((effectIt->mEffectID != ESM::MagicEffect::Corprus) && (magicEffect->mData.mFlags & ESM::MagicEffect::UncappedDamage)) // APPLIED_ONCE
            {
                float random = 1.f;
                if (mSpells[spell].find(i) != mSpells[spell].end())
                    random = mSpells[spell].at(i);

                float magnitude = effectIt->mMagnMin + (effectIt->mMagnMax - effectIt->mMagnMin) * random;
                magnitude *= std::max(1, mCorprusSpells[spell].mWorsenings);
                mPermanentSpellEffects[spell].add(MWMechanics::EffectKey(*effectIt), MWMechanics::EffectParam(magnitude));
            }
        }
    }

    bool Spells::hasCorprusEffect(const ESM::Spell *spell)
    {
        for (std::vector<ESM::ENAMstruct>::const_iterator effectIt = spell->mEffects.mList.begin(); effectIt != spell->mEffects.mList.end(); ++effectIt)
        {
            if (effectIt->mEffectID == ESM::MagicEffect::Corprus)
            {
                return true;
            }
        }
        return false;
    }

    const std::map<Spells::SpellKey, Spells::CorprusStats> &Spells::getCorprusSpells() const
    {
        return mCorprusSpells;
    }

    bool Spells::canUsePower(const ESM::Spell* spell) const
    {
        std::map<SpellKey, MWWorld::TimeStamp>::const_iterator it = mUsedPowers.find(spell);
        if (it == mUsedPowers.end() || it->second + 24 <= MWBase::Environment::get().getWorld()->getTimeStamp())
            return true;
        else
            return false;
    }

    void Spells::usePower(const ESM::Spell* spell)
    {
        mUsedPowers[spell] = MWBase::Environment::get().getWorld()->getTimeStamp();
    }

    void Spells::readState(const ESM::SpellState &state)
    {
        for (ESM::SpellState::TContainer::const_iterator it = state.mSpells.begin(); it != state.mSpells.end(); ++it)
        {
            // Discard spells that are no longer available due to changed content files
            const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search(it->first);
            if (spell)
            {
                mSpells[spell] = it->second;

                if (it->first == state.mSelectedSpell)
                    mSelectedSpell = it->first;
            }
        }

        for (std::map<std::string, ESM::TimeStamp>::const_iterator it = state.mUsedPowers.begin(); it != state.mUsedPowers.end(); ++it)
        {
            const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search(it->first);
            if (!spell)
                continue;
            mUsedPowers[spell] = MWWorld::TimeStamp(it->second);
        }

        for (std::map<std::string, std::vector<ESM::SpellState::PermanentSpellEffectInfo> >::const_iterator it =
            state.mPermanentSpellEffects.begin(); it != state.mPermanentSpellEffects.end(); ++it)
        {
            const ESM::Spell * spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search(it->first);
            if (!spell)
                continue;

            mPermanentSpellEffects[spell] = MagicEffects();
            for (std::vector<ESM::SpellState::PermanentSpellEffectInfo>::const_iterator effectIt = it->second.begin(); effectIt != it->second.end(); ++effectIt)
            {
                mPermanentSpellEffects[spell].add(EffectKey(effectIt->mId, effectIt->mArg), effectIt->mMagnitude);
            }
        }

        mCorprusSpells.clear();
        for (std::map<std::string, ESM::SpellState::CorprusStats>::const_iterator it = state.mCorprusSpells.begin(); it != state.mCorprusSpells.end(); ++it)
        {
            const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search(it->first);
            if (!spell) // Discard unavailable corprus spells
                continue;
            mCorprusSpells[spell].mWorsenings = state.mCorprusSpells.at(it->first).mWorsenings;
            mCorprusSpells[spell].mNextWorsening = MWWorld::TimeStamp(state.mCorprusSpells.at(it->first).mNextWorsening);
        }
    }

    void Spells::writeState(ESM::SpellState &state) const
    {
        for (TContainer::const_iterator it = mSpells.begin(); it != mSpells.end(); ++it)
            state.mSpells.insert(std::make_pair(it->first->mId, it->second));

        state.mSelectedSpell = mSelectedSpell;

        for (std::map<SpellKey, MWWorld::TimeStamp>::const_iterator it = mUsedPowers.begin(); it != mUsedPowers.end(); ++it)
            state.mUsedPowers[it->first->mId] = it->second.toEsm();

        for (std::map<SpellKey, MagicEffects>::const_iterator it = mPermanentSpellEffects.begin(); it != mPermanentSpellEffects.end(); ++it)
        {
            std::vector<ESM::SpellState::PermanentSpellEffectInfo> effectList;
            for (MagicEffects::Collection::const_iterator effectIt = it->second.begin(); effectIt != it->second.end(); ++effectIt)
            {
                ESM::SpellState::PermanentSpellEffectInfo info;
                info.mId = effectIt->first.mId;
                info.mArg = effectIt->first.mArg;
                info.mMagnitude = effectIt->second.getModifier();

                effectList.push_back(info);
            }
            state.mPermanentSpellEffects[it->first->mId] = effectList;
        }

        for (std::map<SpellKey, CorprusStats>::const_iterator it = mCorprusSpells.begin(); it != mCorprusSpells.end(); ++it)
        {
            state.mCorprusSpells[it->first->mId].mWorsenings = mCorprusSpells.at(it->first).mWorsenings;
            state.mCorprusSpells[it->first->mId].mNextWorsening = mCorprusSpells.at(it->first).mNextWorsening.toEsm();
        }
    }
}
