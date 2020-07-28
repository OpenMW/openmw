#ifndef GAME_MWMECHANICS_SPELLLIST_H
#define GAME_MWMECHANICS_SPELLLIST_H

#include <functional>
#include <map>
#include <string>
#include <set>
#include <vector>

#include <components/esm/loadspel.hpp>

#include "magiceffects.hpp"

namespace ESM
{
    struct SpellState;
}

namespace MWMechanics
{
    struct SpellParams
    {
        std::map<int, float> mEffectRands; // <effect index, normalised random magnitude>
        std::set<int> mPurgedEffects; // indices of purged effects
    };

    class Spells;

    class SpellList
    {
            const std::string mId;
            const int mType;
            std::vector<Spells*> mListeners;

            bool withBaseRecord(const std::function<bool(std::vector<std::string>&)>& function);
        public:
            SpellList(const std::string& id, int type);

            /// Get spell from ID, throws exception if not found
            static const ESM::Spell* getSpell(const std::string& id);

            void add (const ESM::Spell* spell);
            ///< Adding a spell that is already listed in *this is a no-op.

            void remove (const ESM::Spell* spell);

            void removeAll(const std::vector<std::string>& spells);

            void clear();
            ///< Remove all spells of all types.

            void addListener(Spells* spells);

            void removeListener(Spells* spells);

            const std::vector<std::string> getSpells() const;
    };
}

#endif
