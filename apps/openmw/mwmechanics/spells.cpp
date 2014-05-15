
#include "spells.hpp"

#include <cstdlib>

#include <components/esm/loadspel.hpp>
#include <components/esm/spellstate.hpp>

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

            std::map<const int, float> random;

            // Determine the random magnitudes (unless this is a castable spell, in which case
            // they will be determined when the spell is cast)
            if (spell->mData.mType != ESM::Spell::ST_Power && spell->mData.mType != ESM::Spell::ST_Spell)
            {
                for (unsigned int i=0; i<spell->mEffects.mList.size();++i)
                {
                    if (spell->mEffects.mList[i].mMagnMin != spell->mEffects.mList[i].mMagnMax)
                        random[i] = static_cast<float> (std::rand()) / RAND_MAX;
                }
            }

            mSpells.insert (std::make_pair (Misc::StringUtils::lowerCase(spellId), random));
        }
    }

    void Spells::remove (const std::string& spellId)
    {
        std::string lower = Misc::StringUtils::lowerCase(spellId);
        TContainer::iterator iter = mSpells.find (lower);

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

            if (spell->mData.mType == ESM::Spell::ST_Blight)
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

            if (Misc::StringUtils::ciEqual(spell->mId, "corprus"))
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
                visitor.visit(MWMechanics::EffectKey(*effectIt), spell->mName, -1, magnitude);
            }
        }
    }

    bool Spells::canUsePower(const std::string &power) const
    {
        std::map<std::string, MWWorld::TimeStamp>::const_iterator it = mUsedPowers.find(power);
        if (it == mUsedPowers.end() || it->second + 24 <= MWBase::Environment::get().getWorld()->getTimeStamp())
            return true;
        else
            return false;
    }

    void Spells::usePower(const std::string &power)
    {
        mUsedPowers[power] = MWBase::Environment::get().getWorld()->getTimeStamp();
    }

    void Spells::readState(const ESM::SpellState &state)
    {
        mSpells = state.mSpells;
        mSelectedSpell = state.mSelectedSpell;

        // Discard spells that are no longer available due to changed content files
        for (TContainer::iterator iter = mSpells.begin(); iter!=mSpells.end();)
        {
            const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search(iter->first);
            if (!spell)
            {
                if (iter->first == mSelectedSpell)
                    mSelectedSpell = "";
                mSpells.erase(iter++);
            }
            else
                ++iter;
        }

        // No need to discard spells here (doesn't really matter if non existent ids are kept)
        for (std::map<std::string, ESM::TimeStamp>::const_iterator it = state.mUsedPowers.begin(); it != state.mUsedPowers.end(); ++it)
            mUsedPowers[it->first] = MWWorld::TimeStamp(it->second);
    }

    void Spells::writeState(ESM::SpellState &state) const
    {
        state.mSpells = mSpells;
        state.mSelectedSpell = mSelectedSpell;

        for (std::map<std::string, MWWorld::TimeStamp>::const_iterator it = mUsedPowers.begin(); it != mUsedPowers.end(); ++it)
            state.mUsedPowers[it->first] = it->second.toEsm();
    }
}
