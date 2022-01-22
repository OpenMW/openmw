#include "spells.hpp"

#include <components/debug/debuglog.hpp>
#include <components/esm3/loadspel.hpp>
#include <components/esm3/spellstate.hpp>
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
    {
    }

    Spells::Spells(const Spells& spells) : mSpellList(spells.mSpellList), mSpells(spells.mSpells),
        mSelectedSpell(spells.mSelectedSpell), mUsedPowers(spells.mUsedPowers)
    {
        if(mSpellList)
            mSpellList->addListener(this);
    }

    Spells::Spells(Spells&& spells) : mSpellList(std::move(spells.mSpellList)), mSpells(std::move(spells.mSpells)),
        mSelectedSpell(std::move(spells.mSelectedSpell)), mUsedPowers(std::move(spells.mUsedPowers))
    {
        if (mSpellList)
            mSpellList->updateListener(&spells, this);
    }

    std::vector<const ESM::Spell*>::const_iterator Spells::begin() const
    {
        return mSpells.begin();
    }

    std::vector<const ESM::Spell*>::const_iterator Spells::end() const
    {
        return mSpells.end();
    }

    bool Spells::hasSpell(const std::string &spell) const
    {
        return hasSpell(SpellList::getSpell(spell));
    }

    bool Spells::hasSpell(const ESM::Spell *spell) const
    {
        return std::find(mSpells.begin(), mSpells.end(), spell) != mSpells.end();
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
        if (!hasSpell(spell))
            mSpells.emplace_back(spell);
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
        const auto it = std::find(mSpells.begin(), mSpells.end(), spell);
        if(it != mSpells.end())
            mSpells.erase(it);
    }

    void Spells::removeAllSpells()
    {
        mSpells.clear();
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

    bool Spells::hasDisease(const ESM::Spell::SpellType type) const
    {
        for (const auto spell : mSpells)
        {
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
            const ESM::Spell *spell = *iter;
            if (filter(spell))
            {
                iter = mSpells.erase(iter);
                purged.push_back(spell->mId);
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

        for (const std::string& id : state.mSpells)
        {
            // Discard spells that are no longer available due to changed content files
            const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search(id);
            if (spell)
            {
                addSpell(spell);

                if (id == state.mSelectedSpell)
                    mSelectedSpell = id;
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
        for (const auto spell : mSpells)
        {
            // Don't save spells and powers stored in the base record
            if((spell->mData.mType != ESM::Spell::ST_Spell && spell->mData.mType != ESM::Spell::ST_Power) ||
                std::find(baseSpells.begin(), baseSpells.end(), spell->mId) == baseSpells.end())
            {
                state.mSpells.emplace_back(spell->mId);
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
