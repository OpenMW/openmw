#ifndef GAME_MWMECHANICS_SPELLS_H
#define GAME_MWMECHANICS_SPELLS_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "../mwworld/timestamp.hpp"

#include "spelllist.hpp"

namespace ESM
{
    struct SpellState;
}

namespace MWMechanics
{
    class CreatureStats;

    class MagicEffects;

    /// \brief Spell list
    ///
    /// This class manages known spells as well as abilities, powers and permanent negative effects like
    /// diseases. It also keeps track of used powers (which can only be used every 24h).
    class Spells
    {
        std::shared_ptr<SpellList> mSpellList;
        std::vector<const ESM::Spell*> mSpells;

        // Note: this is the spell that's about to be cast, *not* the spell selected in the GUI (which may be different)
        ESM::RefId mSelectedSpell;

        std::vector<std::pair<const ESM::Spell*, MWWorld::TimeStamp>> mUsedPowers;

        bool hasSpellType(const ESM::Spell::SpellType type) const;

        using SpellFilter = bool (*)(const ESM::Spell*);
        void purge(const SpellFilter& filter);

        void addSpell(const ESM::Spell* spell);
        void removeSpell(const ESM::Spell* spell);
        void removeAllSpells();

        friend class SpellList;

    public:
        using Collection = std::vector<const ESM::Spell*>;

        Spells();

        Spells(const Spells&);

        Spells(Spells&& spells);

        ~Spells();

        static bool hasCorprusEffect(const ESM::Spell* spell);

        bool canUsePower(const ESM::Spell* spell) const;
        void usePower(const ESM::Spell* spell);

        void purgeCommonDisease();
        void purgeBlightDisease();
        void purgeCorprusDisease();
        void purgeCurses();

        Collection::const_iterator begin() const;

        Collection::const_iterator end() const;

        bool hasSpell(const ESM::RefId& spell) const;
        bool hasSpell(const ESM::Spell* spell) const;

        void add(const ESM::RefId& spell, bool modifyBase = true);
        ///< Adding a spell that is already listed in *this is a no-op.

        void add(const ESM::Spell* spell, bool modifyBase = true);
        ///< Adding a spell that is already listed in *this is a no-op.

        void remove(const ESM::RefId& spell, bool modifyBase = true);
        void remove(const ESM::Spell* spell, bool modifyBase = true);
        ///< If the spell to be removed is the selected spell, the selected spell will be changed to
        /// no spell (empty id).

        void clear(bool modifyBase = false);
        ///< Remove all spells of all types.

        void setSelectedSpell(const ESM::RefId& spellId);
        ///< This function does not verify, if the spell is available.

        const ESM::RefId& getSelectedSpell() const;
        ///< May return an empty id.

        bool hasCommonDisease() const;

        bool hasBlightDisease() const;

        /// Iteration methods for lua
        size_t count() const { return mSpells.size(); }
        const ESM::Spell* at(size_t index) const { return mSpells.at(index); }

        void readState(const ESM::SpellState& state, CreatureStats* creatureStats);
        void writeState(ESM::SpellState& state) const;

        bool setSpells(const ESM::RefId& id);

        void addAllToInstance(const std::vector<ESM::RefId>& spells);
    };
}

#endif
