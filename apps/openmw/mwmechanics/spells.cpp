
#include "spells.hpp"

#include <components/esm_store/store.hpp>

#include <components/esm/loadspel.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "magiceffects.hpp"

namespace MWMechanics
{
    void Spells::addSpell (const ESM::Spell *spell, MagicEffects& effects) const
    {
        effects.add (spell->mEffects);
    }

    Spells::TIterator Spells::begin() const
    {
        return mSpells.begin();
    }

    Spells::TIterator Spells::end() const
    {
        return mSpells.end();
    }

    void Spells::add (const std::string& spellId)
    {
        if (std::find (mSpells.begin(), mSpells.end(), spellId)==mSpells.end())
            mSpells.push_back (spellId);
    }

    void Spells::remove (const std::string& spellId)
    {
        TContainer::iterator iter = std::find (mSpells.begin(), mSpells.end(), spellId);

        if (iter!=mSpells.end())
            mSpells.erase (iter);

        if (spellId==mSelectedSpell)
            mSelectedSpell.clear();
    }

    MagicEffects Spells::getMagicEffects() const
    {
        MagicEffects effects;

        for (TIterator iter = mSpells.begin(); iter!=mSpells.end(); ++iter)
        {
            const ESM::Spell *spell = MWBase::Environment::get().getWorld()->getStore().spells.find (*iter);

            if (spell->mData.mType==ESM::Spell::ST_Ability || spell->mData.mType==ESM::Spell::ST_Blight ||
                spell->mData.mType==ESM::Spell::ST_Disease || spell->mData.mType==ESM::Spell::ST_Curse)
                addSpell (spell, effects);
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
}
