#include "spelllist.hpp"

#include <algorithm>

#include <components/esm3/loadcrea.hpp>
#include <components/esm3/loadnpc.hpp>
#include <components/esm3/loadspel.hpp>

#include "spells.hpp"

#include "../mwbase/environment.hpp"

#include "../mwworld/esmstore.hpp"

namespace
{
    constexpr int sAutoCalc = ESM::REC_NPC_ + 1;

    template <class T>
    const std::vector<ESM::RefId>& getSpellList(ESM::RefId id)
    {
        return MWBase::Environment::get().getESMStore()->get<T>().find(id)->mSpells.mList;
    }

    template <class T>
    bool withBaseRecord(ESM::RefId id, auto&& function)
    {
        T copy = *MWBase::Environment::get().getESMStore()->get<T>().find(id);
        bool changed = function(copy.mSpells.mList);
        if (changed)
            MWBase::Environment::get().getESMStore()->overrideRecord(copy);
        return changed;
    }

    bool withBaseRecord(ESM::RefId id, int type, std::vector<ESM::RefId>& spells, auto&& function)
    {
        switch (type)
        {
            case ESM::REC_CREA:
                return withBaseRecord<ESM::Creature>(id, function);
            case ESM::REC_NPC_:
                return withBaseRecord<ESM::NPC>(id, function);
            case sAutoCalc:
                return function(spells);
            default:
                throw std::logic_error("failed to update base record for " + id.toDebugString());
        }
    }
}

namespace MWMechanics
{
    SpellList::SpellList(ESM::RefId id, int type, bool autoCalc)
        : mId(id)
        , mType(autoCalc ? sAutoCalc : type)
    {
    }

    const std::vector<ESM::RefId>& SpellList::getSpells() const
    {
        switch (mType)
        {
            case ESM::REC_CREA:
                return getSpellList<ESM::Creature>(mId);
            case ESM::REC_NPC_:
                return getSpellList<ESM::NPC>(mId);
            case sAutoCalc:
                return mSpells;
            default:
                throw std::logic_error("failed to get spell list for " + mId.toDebugString());
        }
    }

    void SpellList::setAutoCalc(const std::vector<const ESM::Spell*> spells)
    {
        assert(mType == sAutoCalc);
        for (const ESM::Spell* spell : spells)
            mSpells.push_back(spell->mId);
    }

    void SpellList::add(const ESM::Spell* spell)
    {
        auto& id = spell->mId;
        bool changed = withBaseRecord(mId, mType, mSpells, [&](auto& spells) {
            for (const auto& it : spells)
            {
                if (id == it)
                    return false;
            }
            spells.push_back(id);
            return true;
        });
        if (changed)
        {
            for (auto listener : mListeners)
                listener->addSpell(spell);
        }
    }

    void SpellList::remove(const ESM::Spell* spell)
    {
        auto& id = spell->mId;
        bool changed = withBaseRecord(mId, mType, mSpells, [&](auto& spells) {
            for (auto it = spells.begin(); it != spells.end(); it++)
            {
                if (id == *it)
                {
                    spells.erase(it);
                    return true;
                }
            }
            return false;
        });
        if (changed)
        {
            for (auto listener : mListeners)
                listener->removeSpell(spell);
        }
    }

    void SpellList::removeAll(const std::vector<ESM::RefId>& ids)
    {
        bool changed = withBaseRecord(mId, mType, mSpells, [&](auto& spells) {
            const auto it = std::remove_if(spells.begin(), spells.end(), [&](const auto& spell) {
                const auto isSpell = [&](const auto& id) { return spell == id; };
                return ids.end() != std::find_if(ids.begin(), ids.end(), isSpell);
            });
            if (it == spells.end())
                return false;
            spells.erase(it, spells.end());
            return true;
        });
        if (changed)
        {
            const auto& store = MWBase::Environment::get().getESMStore()->get<ESM::Spell>();
            for (auto listener : mListeners)
            {
                for (auto& id : ids)
                {
                    const auto spell = store.find(id);
                    listener->removeSpell(spell);
                }
            }
        }
    }

    void SpellList::clear()
    {
        bool changed = withBaseRecord(mId, mType, mSpells, [](auto& spells) {
            if (spells.empty())
                return false;
            spells.clear();
            return true;
        });
        if (changed)
        {
            for (auto listener : mListeners)
                listener->removeAllSpells();
        }
    }

    void SpellList::addListener(Spells* spells)
    {
        if (std::find(mListeners.begin(), mListeners.end(), spells) != mListeners.end())
            return;
        mListeners.push_back(spells);
    }

    void SpellList::removeListener(Spells* spells)
    {
        const auto it = std::find(mListeners.begin(), mListeners.end(), spells);
        if (it != mListeners.end())
            mListeners.erase(it);
    }

    void SpellList::updateListener(Spells* before, Spells* after)
    {
        const auto it = std::find(mListeners.begin(), mListeners.end(), before);
        if (it == mListeners.end())
            return mListeners.push_back(after);
        *it = after;
    }
}
