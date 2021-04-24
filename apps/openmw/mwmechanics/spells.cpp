#include "spells.hpp"

#include <components/debug/debuglog.hpp>
#include <components/esm/loadspel.hpp>
#include <components/esm/spellstate.hpp>
#include <components/misc/rng.hpp>
#include <components/misc/stringops.hpp>

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

    Spells::Spells(const Spells& spells) : mSpellList(spells.mSpellList), mSpells(spells.mSpells),
        mSelectedSpell(spells.mSelectedSpell), mUsedPowers(spells.mUsedPowers),
        mSpellsChanged(spells.mSpellsChanged), mEffects(spells.mEffects), mSourcedEffects(spells.mSourcedEffects)
    {
        if(mSpellList)
            mSpellList->addListener(this);
    }

    Spells::Spells(Spells&& spells) : mSpellList(std::move(spells.mSpellList)), mSpells(std::move(spells.mSpells)),
        mSelectedSpell(std::move(spells.mSelectedSpell)), mUsedPowers(std::move(spells.mUsedPowers)),
        mSpellsChanged(std::move(spells.mSpellsChanged)), mEffects(std::move(spells.mEffects)),
        mSourcedEffects(std::move(spells.mSourcedEffects))
    {
        if (mSpellList)
            mSpellList->updateListener(&spells, this);
    }

    std::map<const ESM::Spell*, SpellParams>::const_iterator Spells::begin() const
    {
        return mSpells.begin();
    }

    std::map<const ESM::Spell*, SpellParams>::const_iterator Spells::end() const
    {
        return mSpells.end();
    }

    void Spells::rebuildEffects() const
    {
        mEffects = MagicEffects();
        mSourcedEffects.clear();

        for (const auto& iter : mSpells)
        {
            const ESM::Spell *spell = iter.first;

            if (spell->mData.mType==ESM::Spell::ST_Ability || spell->mData.mType==ESM::Spell::ST_Blight ||
                spell->mData.mType==ESM::Spell::ST_Disease || spell->mData.mType==ESM::Spell::ST_Curse)
            {
                int i=0;
                for (const auto& effect : spell->mEffects.mList)
                {
                    if (iter.second.mPurgedEffects.find(i) != iter.second.mPurgedEffects.end())
                    {
                        ++i;
                        continue; // effect was purged
                    }

                    float random = 1.f;
                    if (iter.second.mEffectRands.find(i) != iter.second.mEffectRands.end())
                        random = iter.second.mEffectRands.at(i);

                    float magnitude = effect.mMagnMin + (effect.mMagnMax - effect.mMagnMin) * random;
                    mEffects.add (effect, magnitude);
                    mSourcedEffects[spell].add(MWMechanics::EffectKey(effect), magnitude);

                    ++i;
                }
            }
        }
    }

    bool Spells::hasSpell(const std::string &spell) const
    {
        return hasSpell(SpellList::getSpell(spell));
    }

    bool Spells::hasSpell(const ESM::Spell *spell) const
    {
        return mSpells.find(spell) != mSpells.end();
    }

    void Spells::add (const ESM::Spell* spell)
    {
        mSpellList->add(spell);
    }

    void Spells::add (const std::string& spellId)
    {
        add(SpellList::getSpell(spellId));
    }

    void Spells::addSpell(const ESM::Spell* spell)
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
            mSpells.emplace(spell, params);
            mSpellsChanged = true;
        }
    }

    void Spells::remove (const std::string& spellId)
    {
        const auto spell = SpellList::getSpell(spellId);
        removeSpell(spell);
        mSpellList->remove(spell);

        if (spellId==mSelectedSpell)
            mSelectedSpell.clear();
    }

    void Spells::removeSpell(const ESM::Spell* spell)
    {
        const auto it = mSpells.find(spell);
        if(it != mSpells.end())
        {
            mSpells.erase(it);
            mSpellsChanged = true;
        }
    }

    MagicEffects Spells::getMagicEffects() const
    {
        if (mSpellsChanged) {
            rebuildEffects();
            mSpellsChanged = false;
        }
        return mEffects;
    }

    void Spells::removeAllSpells()
    {
        mSpells.clear();
        mSpellsChanged = true;
    }

    void Spells::clear(bool modifyBase)
    {
        removeAllSpells();
        if(modifyBase)
            mSpellList->clear();
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

    bool Spells::hasDisease(const ESM::Spell::SpellType type) const
    {
        for (const auto& iter : mSpells)
        {
            const ESM::Spell *spell = iter.first;
            if (spell->mData.mType == type)
                return true;
        }

        return false;
    }

    bool Spells::hasCommonDisease() const
    {
        return hasDisease(ESM::Spell::ST_Disease);
    }

    bool Spells::hasBlightDisease() const
    {
        return hasDisease(ESM::Spell::ST_Blight);
    }

    void Spells::purge(const SpellFilter& filter)
    {
        std::vector<std::string> purged;
        for (auto iter = mSpells.begin(); iter!=mSpells.end();)
        {
            const ESM::Spell *spell = iter->first;
            if (filter(spell))
            {
                mSpells.erase(iter++);
                purged.push_back(spell->mId);
                mSpellsChanged = true;
            }
            else
                ++iter;
        }
        if(!purged.empty())
            mSpellList->removeAll(purged);
    }

    void Spells::purgeCommonDisease()
    {
        purge([](auto spell) { return spell->mData.mType == ESM::Spell::ST_Disease; });
    }

    void Spells::purgeBlightDisease()
    {
        purge([](auto spell) { return spell->mData.mType == ESM::Spell::ST_Blight && !hasCorprusEffect(spell); });
    }

    void Spells::purgeCorprusDisease()
    {
        purge(&hasCorprusEffect);
    }

    void Spells::purgeCurses()
    {
        purge([](auto spell) { return spell->mData.mType == ESM::Spell::ST_Curse; });
    }

    void Spells::removeEffects(const std::string &id)
    {
        if (isSpellActive(id))
        {
            for (auto& spell : mSpells)
            {
                if (spell.first == SpellList::getSpell(id))
                {
                    for (long unsigned int i = 0; i != spell.first->mEffects.mList.size(); i++)
                    {
                        spell.second.mPurgedEffects.insert(i);
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

        for (const auto& it : mSourcedEffects)
        {
            const ESM::Spell * spell = it.first;
            for (const auto& effectIt : it.second)
            {
                // FIXME: since Spells merges effects with the same ID, there is no sense to use multiple effects with same ID here
                visitor.visit(effectIt.first, -1, spell->mName, spell->mId, -1, effectIt.second.getMagnitude());
            }
        }
    }

    bool Spells::hasCorprusEffect(const ESM::Spell *spell)
    {
        for (const auto& effectIt : spell->mEffects.mList)
        {
            if (effectIt.mEffectID == ESM::MagicEffect::Corprus)
            {
                return true;
            }
        }
        return false;
    }

    void Spells::purgeEffect(int effectId)
    {
        for (auto& spellIt : mSpells)
        {
            int i = 0;
            for (auto& effectIt : spellIt.first->mEffects.mList)
            {
                if (effectIt.mEffectID == effectId)
                {
                    spellIt.second.mPurgedEffects.insert(i);
                    mSpellsChanged = true;
                }
                ++i;
            }
        }
    }

    void Spells::purgeEffect(int effectId, const std::string & sourceId)
    {
        // Effect source may be not a spell
        const ESM::Spell * spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search(sourceId);
        if (spell == nullptr)
            return;

        auto spellIt = mSpells.find(spell);
        if (spellIt == mSpells.end())
            return;

        int index = 0;
        for (auto& effectIt : spellIt->first->mEffects.mList)
        {
            if (effectIt.mEffectID == effectId)
            {
                spellIt->second.mPurgedEffects.insert(index);
                mSpellsChanged = true;
            }
            ++index;
        }
    }

    bool Spells::canUsePower(const ESM::Spell* spell) const
    {
        const auto it = mUsedPowers.find(spell);
        return it == mUsedPowers.end() || it->second + 24 <= MWBase::Environment::get().getWorld()->getTimeStamp();
    }

    void Spells::usePower(const ESM::Spell* spell)
    {
        mUsedPowers[spell] = MWBase::Environment::get().getWorld()->getTimeStamp();
    }

    void Spells::readState(const ESM::SpellState &state, CreatureStats* creatureStats)
    {
        const auto& baseSpells = mSpellList->getSpells();

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
        // Add spells from the base record
        for(const std::string& id : baseSpells)
        {
            const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search(id);
            if(spell)
                addSpell(spell);
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
        const auto& baseSpells = mSpellList->getSpells();
        for (const auto& it : mSpells)
        {
            // Don't save spells and powers stored in the base record
            if((it.first->mData.mType != ESM::Spell::ST_Spell && it.first->mData.mType != ESM::Spell::ST_Power) ||
                std::find(baseSpells.begin(), baseSpells.end(), it.first->mId) == baseSpells.end())
            {
                ESM::SpellState::SpellParams params;
                params.mEffectRands = it.second.mEffectRands;
                params.mPurgedEffects = it.second.mPurgedEffects;
                state.mSpells.emplace(it.first->mId, params);
            }
        }

        state.mSelectedSpell = mSelectedSpell;

        for (const auto& it : mUsedPowers)
            state.mUsedPowers[it.first->mId] = it.second.toEsm();
    }

    bool Spells::setSpells(const std::string& actorId)
    {
        bool result;
        std::tie(mSpellList, result) = MWBase::Environment::get().getWorld()->getStore().getSpellList(actorId);
        mSpellList->addListener(this);
        addAllToInstance(mSpellList->getSpells());
        return result;
    }

    void Spells::addAllToInstance(const std::vector<std::string>& spells)
    {
        for(const std::string& id : spells)
        {
            const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search(id);
            if(spell)
                addSpell(spell);
            else
                Log(Debug::Warning) << "Warning: ignoring nonexistent spell '" << id << "'";
        }
    }

    Spells::~Spells()
    {
        if(mSpellList)
            mSpellList->removeListener(this);
    }
}
