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

    void Spells::add (const ESM::Spell* spell)
    {
        if (mSpells.find (spell->mId)==mSpells.end())
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

                mCorprusSpells[spell->mId] = corprus;
            }

            mSpells.insert (std::make_pair (spell->mId, random));
        }
    }

    void Spells::add (const std::string& spellId)
    {
        const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find(spellId);
        add(spell);
    }

    void Spells::remove (const std::string& spellId)
    {
        std::string lower = Misc::StringUtils::lowerCase(spellId);
        TContainer::iterator iter = mSpells.find (lower);
        std::map<std::string, CorprusStats>::iterator corprusIt = mCorprusSpells.find(lower);

        // if it's corprus, remove negative and keep positive effects
        if (corprusIt != mCorprusSpells.end())
        {
            worsenCorprus(lower);
            if (mPermanentSpellEffects.find(lower) != mPermanentSpellEffects.end())
            {
                MagicEffects & effects = mPermanentSpellEffects[lower];
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
            const ESM::Spell *spell =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find (iter->first);

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

        for (std::map<std::string, MagicEffects>::const_iterator it = mPermanentSpellEffects.begin(); it != mPermanentSpellEffects.end(); ++it)
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
        TContainer::const_iterator found = mSpells.find(id);
        if (found != mSpells.end())
        {
            const ESM::Spell *spell =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find (id);

            return (spell->mData.mType==ESM::Spell::ST_Ability || spell->mData.mType==ESM::Spell::ST_Blight ||
                spell->mData.mType==ESM::Spell::ST_Disease || spell->mData.mType==ESM::Spell::ST_Curse);
        }
        return false;
    }

    bool Spells::hasCommonDisease() const
    {
        for (TIterator iter = mSpells.begin(); iter!=mSpells.end(); ++iter)
        {
            const ESM::Spell *spell =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find (iter->first);

            if (spell->mData.mType == ESM::Spell::ST_Disease)
                return true;
        }

        return false;
    }

    bool Spells::hasBlightDisease() const
    {
        for (TIterator iter = mSpells.begin(); iter!=mSpells.end(); ++iter)
        {
            const ESM::Spell *spell =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find (iter->first);

            if (spell->mData.mType == ESM::Spell::ST_Blight)
                return true;
        }

        return false;
    }

    void Spells::purgeCommonDisease()
    {
        for (TContainer::iterator iter = mSpells.begin(); iter!=mSpells.end();)
        {
            const ESM::Spell *spell =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find (iter->first);

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
            const ESM::Spell *spell =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find (iter->first);

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
            const ESM::Spell *spell =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find (iter->first);

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
            const ESM::Spell *spell =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find (iter->first);

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
            const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find(it->first);

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

    void Spells::worsenCorprus(const std::string &corpSpellId)
    {
        mCorprusSpells[corpSpellId].mNextWorsening = MWBase::Environment::get().getWorld()->getTimeStamp() + CorprusStats::sWorseningPeriod;
        mCorprusSpells[corpSpellId].mWorsenings++;

        // update worsened effects
        mPermanentSpellEffects[corpSpellId] = MagicEffects();
        const ESM::Spell * spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find(corpSpellId);
        int i=0;
        for (std::vector<ESM::ENAMstruct>::const_iterator effectIt = spell->mEffects.mList.begin(); effectIt != spell->mEffects.mList.end(); ++effectIt, ++i)
        {
            const ESM::MagicEffect * magicEffect = MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find(effectIt->mEffectID);
            if ((effectIt->mEffectID != ESM::MagicEffect::Corprus) && (magicEffect->mData.mFlags & ESM::MagicEffect::UncappedDamage)) // APPLIED_ONCE
            {
                float random = 1.f;
                if (mSpells[corpSpellId].find(i) != mSpells[corpSpellId].end())
                    random = mSpells[corpSpellId].at(i);

                float magnitude = effectIt->mMagnMin + (effectIt->mMagnMax - effectIt->mMagnMin) * random;
                magnitude *= std::max(1, mCorprusSpells[corpSpellId].mWorsenings);
                mPermanentSpellEffects[corpSpellId].add(MWMechanics::EffectKey(*effectIt), MWMechanics::EffectParam(magnitude));
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

    const std::map<std::string, Spells::CorprusStats> &Spells::getCorprusSpells() const
    {
        return mCorprusSpells;
    }

    bool Spells::canUsePower(const std::string &power) const
    {
        std::map<std::string, MWWorld::TimeStamp>::const_iterator it = mUsedPowers.find(Misc::StringUtils::lowerCase(power));
        if (it == mUsedPowers.end() || it->second + 24 <= MWBase::Environment::get().getWorld()->getTimeStamp())
            return true;
        else
            return false;
    }

    void Spells::usePower(const std::string &power)
    {
        mUsedPowers[Misc::StringUtils::lowerCase(power)] = MWBase::Environment::get().getWorld()->getTimeStamp();
    }

    void Spells::readState(const ESM::SpellState &state)
    {
        for (TContainer::const_iterator it = state.mSpells.begin(); it != state.mSpells.end(); ++it)
        {
            // Discard spells that are no longer available due to changed content files
            const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search(it->first);
            if (spell)
            {
                mSpells[it->first] = it->second;

                if (it->first == state.mSelectedSpell)
                    mSelectedSpell = it->first;
            }
        }

        // No need to discard spells here (doesn't really matter if non existent ids are kept)
        for (std::map<std::string, ESM::TimeStamp>::const_iterator it = state.mUsedPowers.begin(); it != state.mUsedPowers.end(); ++it)
            mUsedPowers[it->first] = MWWorld::TimeStamp(it->second);

        for (std::map<std::string, std::vector<ESM::SpellState::PermanentSpellEffectInfo> >::const_iterator it =
            state.mPermanentSpellEffects.begin(); it != state.mPermanentSpellEffects.end(); ++it)
        {
            const ESM::Spell * spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search(it->first);
            if (!spell)
                continue;

            mPermanentSpellEffects[it->first] = MagicEffects();
            for (std::vector<ESM::SpellState::PermanentSpellEffectInfo>::const_iterator effectIt = it->second.begin(); effectIt != it->second.end(); ++effectIt)
            {
                mPermanentSpellEffects[it->first].add(EffectKey(effectIt->mId, effectIt->mArg), effectIt->mMagnitude);
            }
        }

        mCorprusSpells.clear();
        for (std::map<std::string, ESM::SpellState::CorprusStats>::const_iterator it = state.mCorprusSpells.begin(); it != state.mCorprusSpells.end(); ++it)
        {
            if (mSpells.find(it->first) != mSpells.end()) // Discard unavailable corprus spells
            {
                mCorprusSpells[it->first].mWorsenings = state.mCorprusSpells.at(it->first).mWorsenings;
                mCorprusSpells[it->first].mNextWorsening = MWWorld::TimeStamp(state.mCorprusSpells.at(it->first).mNextWorsening);
            }
        }
    }

    void Spells::writeState(ESM::SpellState &state) const
    {
        state.mSpells = mSpells;
        state.mSelectedSpell = mSelectedSpell;

        for (std::map<std::string, MWWorld::TimeStamp>::const_iterator it = mUsedPowers.begin(); it != mUsedPowers.end(); ++it)
            state.mUsedPowers[it->first] = it->second.toEsm();

        for (std::map<std::string, MagicEffects>::const_iterator it = mPermanentSpellEffects.begin(); it != mPermanentSpellEffects.end(); ++it)
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
            state.mPermanentSpellEffects[it->first] = effectList;
        }

        for (std::map<std::string, CorprusStats>::const_iterator it = mCorprusSpells.begin(); it != mCorprusSpells.end(); ++it)
        {
            state.mCorprusSpells[it->first].mWorsenings = mCorprusSpells.at(it->first).mWorsenings;
            state.mCorprusSpells[it->first].mNextWorsening = mCorprusSpells.at(it->first).mNextWorsening.toEsm();
        }
    }
}
