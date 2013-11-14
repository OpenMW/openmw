
#include "spells.hpp"

#include <cstdlib>

#include <components/esm/loadspel.hpp>

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

    void Spells::add (const std::string& spellId)
    {
        if (mSpells.find (spellId)==mSpells.end())
        {
            const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find(spellId);

            std::vector<float> random;
            random.resize(spell->mEffects.mList.size());
            for (unsigned int i=0; i<random.size();++i)
                random[i] = static_cast<float> (std::rand()) / RAND_MAX;
            mSpells.insert (std::make_pair (spellId, random));
        }
    }

    void Spells::remove (const std::string& spellId)
    {
        TContainer::iterator iter = mSpells.find (spellId);

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
                    effects.add (*it, iter->second[i]);
                    ++i;
                }
            }
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
}
