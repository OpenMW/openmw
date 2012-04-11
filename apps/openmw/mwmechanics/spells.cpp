
#include "spells.hpp"

#include <cassert>

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

    Spells::TIterator Spells::begin (ESM::Spell::SpellType type) const
    {
        assert (type>=0 && type<sTypes);
        return mSpells[type].begin();
    }

    Spells::TIterator Spells::end (ESM::Spell::SpellType type) const
    {
        assert (type>=0 && type<sTypes);
        return mSpells[type].end();
    }

    void Spells::add (const std::string& spellId, MWWorld::Environment& environment)
    {
        const ESM::Spell *spell = environment.mWorld->getStore().spells.find (spellId);

        int type = spell->data.type;

        assert (type>=0 && type<sTypes);

        if (std::find (mSpells[type].begin(), mSpells[type].end(), spell)!=mSpells[type].end())
            mSpells[type].push_back (spell);
    }

    void Spells::remove (const std::string& spellId, MWWorld::Environment& environment)
    {
        const ESM::Spell *spell = environment.mWorld->getStore().spells.find (spellId);

        int type = spell->data.type;

        TContainer::iterator iter = std::find (mSpells[type].begin(), mSpells[type].end(), spell);

        if (iter!=mSpells[type].end())
            mSpells[type].erase (iter);
    }

    MagicEffects Spells::getMagicEffects (MWWorld::Environment& environment) const
    {
        MagicEffects effects;

        for (int i=ESM::Spell::ST_Ability; i<=ESM::Spell::ST_Curse; ++i)
            for (TIterator iter (begin (static_cast<ESM::Spell::SpellType> (i)));
                iter!=end(static_cast<ESM::Spell::SpellType> (i)); ++iter)
                addSpell (*iter, effects);

        return effects;
    }

    void Spells::clear()
    {
        for (int i=0; i<sTypes; ++i)
            mSpells[i].clear();
    }
}
