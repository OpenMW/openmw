#ifndef GAME_MWMECHANICS_SPELLS_H
#define GAME_MWMECHANICS_SPELLS_H

#include <memory>
#include <map>
#include <string>
#include <vector>

#include "../mwworld/timestamp.hpp"

#include "magiceffects.hpp"
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
            std::string mSelectedSpell;

            std::vector<std::pair<const ESM::Spell*, MWWorld::TimeStamp>> mUsedPowers;

            bool hasSpellType(const ESM::Spell::SpellType type) const;

            using SpellFilter = bool (*)(const ESM::Spell*);
            void purge(const SpellFilter& filter);

            void addSpell(const ESM::Spell* spell);
            void removeSpell(const ESM::Spell* spell);
            void removeAllSpells();

            friend class SpellList;
        public:
            Spells();

            Spells(const Spells&);

            Spells(Spells&& spells);

            ~Spells();

            static bool hasCorprusEffect(const ESM::Spell *spell);

            bool canUsePower (const ESM::Spell* spell) const;
            void usePower (const ESM::Spell* spell);

            void purgeCommonDisease();
            void purgeBlightDisease();
            void purgeCorprusDisease();
            void purgeCurses();

            std::vector<const ESM::Spell*>::const_iterator begin() const;

            std::vector<const ESM::Spell*>::const_iterator end() const;

            bool hasSpell(const std::string& spell) const;
            bool hasSpell(const ESM::Spell* spell) const;

            void add (const std::string& spell);
            ///< Adding a spell that is already listed in *this is a no-op.

            void add (const ESM::Spell* spell);
            ///< Adding a spell that is already listed in *this is a no-op.

            void remove (const std::string& spell);
            ///< If the spell to be removed is the selected spell, the selected spell will be changed to
            /// no spell (empty string).

            void clear(bool modifyBase = false);
            ///< Remove all spells of al types.

            void setSelectedSpell (const std::string& spellId);
            ///< This function does not verify, if the spell is available.

            const std::string getSelectedSpell() const;
            ///< May return an empty string.

            bool hasCommonDisease() const;

            bool hasBlightDisease() const;

            void readState (const ESM::SpellState& state, CreatureStats* creatureStats);
            void writeState (ESM::SpellState& state) const;

            bool setSpells(const std::string& id);

            void addAllToInstance(const std::vector<std::string>& spells);
    };
}

#endif
