#include "spelllist.hpp"

#include <algorithm>

#include <components/esm3/loadcrea.hpp>
#include <components/esm3/loadnpc.hpp>
#include <components/esm3/loadspel.hpp>

#include "spells.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/esmstore.hpp"

namespace
{
    template <class T>
    const std::vector<ESM::RefId> getSpellList(const ESM::RefId& id)
    {
        return MWBase::Environment::get().getWorld()->getStore().get<T>().find(id)->mSpells.mList;
    }

    template <class T>
    bool withBaseRecord(const ESM::RefId& id, const std::function<bool(std::vector<ESM::RefId>&)>& function)
    {
        T copy = *MWBase::Environment::get().getWorld()->getStore().get<T>().find(id);
        bool changed = function(copy.mSpells.mList);
        if (changed)
            MWBase::Environment::get().getWorld()->createOverrideRecord(copy);
        return changed;
    }
}

namespace MWMechanics
{
    SpellList::SpellList(const ESM::RefId& id, int type)
        : mId(id)
        , mType(type)
    {
    }

    bool SpellList::withBaseRecord(const std::function<bool(std::vector<ESM::RefId>&)>& function)
    {
        switch (mType)
        {
            case ESM::REC_CREA:
                return ::withBaseRecord<ESM::Creature>(mId, function);
            case ESM::REC_NPC_:
                return ::withBaseRecord<ESM::NPC>(mId, function);
            default:
                throw std::logic_error("failed to update base record for " + mId.toDebugString());
        }
    }

    const std::vector<ESM::RefId> SpellList::getSpells() const
    {
        switch (mType)
        {
            case ESM::REC_CREA:
                return getSpellList<ESM::Creature>(mId);
            case ESM::REC_NPC_:
                return getSpellList<ESM::NPC>(mId);
            default:
                throw std::logic_error("failed to get spell list for " + mId.toDebugString());
        }
    }

    const ESM::Spell* SpellList::getSpell(const ESM::RefId& id)
    {
        return MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find(id);
    }

    void SpellList::add(const ESM::Spell* spell)
    {
        auto& id = spell->mId;
        bool changed = withBaseRecord([&](auto& spells) {
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
        bool changed = withBaseRecord([&](auto& spells) {
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
        bool changed = withBaseRecord([&](auto& spells) {
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
            for (auto listener : mListeners)
            {
                for (auto& id : ids)
                {
                    const auto spell = getSpell(id);
                    listener->removeSpell(spell);
                }
            }
        }
    }

    void SpellList::clear()
    {
        bool changed = withBaseRecord([](auto& spells) {
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
