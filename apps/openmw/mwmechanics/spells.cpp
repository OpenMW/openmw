#include "spells.hpp"

#include <components/esm/loadspel.hpp>
#include <components/esm/spellstate.hpp>
#include <components/misc/rng.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"

#include "actorutil.hpp"
#include "creaturestats.hpp"
#include "magiceffects.hpp"
#include "stat.hpp"

namespace MWMechanics
{
    Spells::Spells()
        : mSpellsChanged(false)
    {
    }

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

    void Spells::rebuildEffects() const
    {
        mEffects = MagicEffects();
        mSourcedEffects.clear();

        for (TIterator iter = mSpells.begin(); iter!=mSpells.end(); ++iter)
        {
            const ESM::Spell *spell = iter->first;

            if (spell->mData.mType==ESM::Spell::ST_Ability || spell->mData.mType==ESM::Spell::ST_Blight ||
                spell->mData.mType==ESM::Spell::ST_Disease || spell->mData.mType==ESM::Spell::ST_Curse)
            {
                int i=0;
                for (std::vector<ESM::ENAMstruct>::const_iterator it = spell->mEffects.mList.begin(); it != spell->mEffects.mList.end(); ++it)
                {
                    if (iter->second.mPurgedEffects.find(i) != iter->second.mPurgedEffects.end())
                        continue; // effect was purged

                    float random = 1.f;
                    if (iter->second.mEffectRands.find(i) != iter->second.mEffectRands.end())
                        random = iter->second.mEffectRands.at(i);

                    float magnitude = it->mMagnMin + (it->mMagnMax - it->mMagnMin) * random;
                    mEffects.add (*it, magnitude);
                    mSourcedEffects[spell].add(MWMechanics::EffectKey(*it), magnitude);

                    ++i;
                }
            }
        }
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
            std::map<int, float> random;

            // Determine the random magnitudes (unless this is a castable spell, in which case
            // they will be determined when the spell is cast)
            if (spell->mData.mType != ESM::Spell::ST_Power && spell->mData.mType != ESM::Spell::ST_Spell)
            {
                for (unsigned int i=0; i<spell->mEffects.mList.size();++i)
                {
                    if (spell->mEffects.mList[i].mMagnMin != spell->mEffects.mList[i].mMagnMax)
                    {
                        int delta = spell->mEffects.mList[i].mMagnMax - spell->mEffects.mList[i].mMagnMin;
                        random[i] = Misc::Rng::rollDice(delta + 1) / static_cast<float>(delta);
                    }
                }
            }

            SpellParams params;
            params.mEffectRands = random;
            mSpells.insert (std::make_pair (spell, params));
            mSpellsChanged = true;
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

        if (iter!=mSpells.end())
        {
            mSpells.erase (iter);
            mSpellsChanged = true;
        }

        if (spellId==mSelectedSpell)
            mSelectedSpell.clear();
    }

    MagicEffects Spells::getMagicEffects() const
    {
        if (mSpellsChanged) {
            rebuildEffects();
            mSpellsChanged = false;
        }
        return mEffects;
    }

    void Spells::clear()
    {
        mSpells.clear();
        mSpellsChanged = true;
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
        if (id.empty())
            return false;

        const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search(id);
        if (spell && hasSpell(spell))
        {
            auto type = spell->mData.mType;
            return (type==ESM::Spell::ST_Ability || type==ESM::Spell::ST_Blight || type==ESM::Spell::ST_Disease || type==ESM::Spell::ST_Curse);
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
            {
                mSpells.erase(iter++);
                mSpellsChanged = true;
            }
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
            {
                mSpells.erase(iter++);
                mSpellsChanged = true;
            }
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
            {
                mSpells.erase(iter++);
                mSpellsChanged = true;
            }
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
            {
                mSpells.erase(iter++);
                mSpellsChanged = true;
            }
            else
                ++iter;
        }
    }

    void Spells::removeEffects(const std::string &id)
    {
        if (isSpellActive(id))
        {
            for (TContainer::iterator spell = mSpells.begin(); spell != mSpells.end(); ++spell)
            {
                if (spell->first == getSpell(id))
                {
                    for (long unsigned int i = 0; i != spell->first->mEffects.mList.size(); i++)
                    {
                        spell->second.mPurgedEffects.insert(i);
                    }
                }
            }

            mSpellsChanged = true;
        }
    }

    void Spells::visitEffectSources(EffectSourceVisitor &visitor) const
    {
        if (mSpellsChanged) {
            rebuildEffects();
            mSpellsChanged = false;
        }

        for (std::map<SpellKey, MagicEffects>::const_iterator it = mSourcedEffects.begin();
             it != mSourcedEffects.end(); ++it)
        {
            const ESM::Spell * spell = it->first;
            for (MagicEffects::Collection::const_iterator effectIt = it->second.begin();
                 effectIt != it->second.end(); ++effectIt)
            {
                visitor.visit(effectIt->first, spell->mName, spell->mId, -1, effectIt->second.getMagnitude());
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

    void Spells::purgeEffect(int effectId)
    {
        for (TContainer::iterator spellIt = mSpells.begin(); spellIt != mSpells.end(); ++spellIt)
        {
            int i = 0;
            for (std::vector<ESM::ENAMstruct>::const_iterator effectIt = spellIt->first->mEffects.mList.begin(); effectIt != spellIt->first->mEffects.mList.end(); ++effectIt)
            {
                if (effectIt->mEffectID == effectId)
                {
                    spellIt->second.mPurgedEffects.insert(i);
                    mSpellsChanged = true;
                }
                ++i;
            }
        }
    }

    void Spells::purgeEffect(int effectId, const std::string & sourceId)
    {
        const ESM::Spell * spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find(sourceId);
        TContainer::iterator spellIt = mSpells.find(spell);
        if (spellIt == mSpells.end())
            return;

        int i = 0;
        for (std::vector<ESM::ENAMstruct>::const_iterator effectIt = spellIt->first->mEffects.mList.begin(); effectIt != spellIt->first->mEffects.mList.end(); ++effectIt)
        {
            if (effectIt->mEffectID == effectId)
            {
                spellIt->second.mPurgedEffects.insert(i);
                mSpellsChanged = true;
            }
            ++i;
        }
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

    void Spells::readState(const ESM::SpellState &state, CreatureStats* creatureStats)
    {
        for (ESM::SpellState::TContainer::const_iterator it = state.mSpells.begin(); it != state.mSpells.end(); ++it)
        {
            // Discard spells that are no longer available due to changed content files
            const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search(it->first);
            if (spell)
            {
                mSpells[spell].mEffectRands = it->second.mEffectRands;
                mSpells[spell].mPurgedEffects = it->second.mPurgedEffects;

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

        for (std::map<std::string, ESM::SpellState::CorprusStats>::const_iterator it = state.mCorprusSpells.begin(); it != state.mCorprusSpells.end(); ++it)
        {
            const ESM::Spell * spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search(it->first);
            if (!spell)
                continue;

            CorprusStats stats;

            int worsening = state.mCorprusSpells.at(it->first).mWorsenings;

            for (int i=0; i<ESM::Attribute::Length; ++i)
                stats.mWorsenings[i] = 0;

            for (auto& effect : spell->mEffects.mList)
            {
                if (effect.mEffectID == ESM::MagicEffect::DrainAttribute)
                    stats.mWorsenings[effect.mAttribute] = worsening;
            }
            stats.mNextWorsening = MWWorld::TimeStamp(state.mCorprusSpells.at(it->first).mNextWorsening);

            creatureStats->addCorprusSpell(it->first, stats);
        }

        mSpellsChanged = true;

        // Permanent effects are used only to keep the custom magnitude of corprus spells effects (after cure too), and only in old saves. Convert data to the new approach.
        for (std::map<std::string, std::vector<ESM::SpellState::PermanentSpellEffectInfo> >::const_iterator it =
            state.mPermanentSpellEffects.begin(); it != state.mPermanentSpellEffects.end(); ++it)
        {
            const ESM::Spell * spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search(it->first);
            if (!spell)
                continue;

            // Import data only for player, other actors should not suffer from corprus worsening.
            MWWorld::Ptr player = getPlayer();
            if (creatureStats->getActorId() != player.getClass().getCreatureStats(player).getActorId())
                return;

            // Note: if target actor has the Restore attirbute effects, stats will be restored.
            for (std::vector<ESM::SpellState::PermanentSpellEffectInfo>::const_iterator effectIt = it->second.begin(); effectIt != it->second.end(); ++effectIt)
            {
                // Applied corprus effects are already in loaded stats modifiers
                if (effectIt->mId == ESM::MagicEffect::FortifyAttribute)
                {
                    AttributeValue attr = creatureStats->getAttribute(effectIt->mArg);
                    attr.setModifier(attr.getModifier() - effectIt->mMagnitude);
                    attr.damage(-effectIt->mMagnitude);
                    creatureStats->setAttribute(effectIt->mArg, attr);
                }
                else if (effectIt->mId == ESM::MagicEffect::DrainAttribute)
                {
                    AttributeValue attr = creatureStats->getAttribute(effectIt->mArg);
                    attr.setModifier(attr.getModifier() + effectIt->mMagnitude);
                    attr.damage(effectIt->mMagnitude);
                    creatureStats->setAttribute(effectIt->mArg, attr);
                }
            }
        }
    }

    void Spells::writeState(ESM::SpellState &state) const
    {
        for (TContainer::const_iterator it = mSpells.begin(); it != mSpells.end(); ++it)
        {
            ESM::SpellState::SpellParams params;
            params.mEffectRands = it->second.mEffectRands;
            params.mPurgedEffects = it->second.mPurgedEffects;
            state.mSpells.insert(std::make_pair(it->first->mId, params));
        }

        state.mSelectedSpell = mSelectedSpell;

        for (std::map<SpellKey, MWWorld::TimeStamp>::const_iterator it = mUsedPowers.begin(); it != mUsedPowers.end(); ++it)
            state.mUsedPowers[it->first->mId] = it->second.toEsm();
    }
}
