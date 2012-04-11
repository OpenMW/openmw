
#include "spells.hpp"

#include <components/esm/loadspel.hpp>

#include "../mwworld/environment.hpp"
#include "../mwworld/world.hpp"

#include "magiceffects.hpp"

namespace MWMechanics
{
    void Spells::addSpell (const ESM::Spell *spell, MagicEffects& effects) const
    {
        for (std::vector<ESM::ENAMstruct>::const_iterator iter = spell->effects.list.begin();
            iter!=spell->effects.list.end(); ++iter)
        {
            EffectParam param;
            param.mMagnitude = iter->magnMax; /// \todo calculate magnitude
            effects.add (EffectKey (*iter), param);
        }
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
        if (std::find (mSpells.begin(), mSpells.end(), spellId)!=mSpells.end())
            mSpells.push_back (spellId);
    }

    void Spells::remove (const std::string& spellId)
    {
        TContainer::iterator iter = std::find (mSpells.begin(), mSpells.end(), spellId);

        if (iter!=mSpells.end())
            mSpells.erase (iter);
    }

    MagicEffects Spells::getMagicEffects (const MWWorld::Environment& environment) const
    {
        MagicEffects effects;

        for (TIterator iter = mSpells.begin(); iter!=mSpells.end(); ++iter)
        {
            const ESM::Spell *spell = environment.mWorld->getStore().spells.find (*iter);

            if (spell->data.type==ESM::Spell::ST_Ability || spell->data.type==ESM::Spell::ST_Blight ||
                spell->data.type==ESM::Spell::ST_Disease || spell->data.type==ESM::Spell::ST_Curse)
                addSpell (spell, effects);
        }

        return effects;
    }

    void Spells::clear()
    {
        mSpells.clear();
    }
}
