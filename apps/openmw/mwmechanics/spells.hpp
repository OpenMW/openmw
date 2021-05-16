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
            std::map<const ESM::Spell*, SpellParams> mSpells;

            // Note: this is the spell that's about to be cast, *not* the spell selected in the GUI (which may be different)
            std::string mSelectedSpell;

            std::map<const ESM::Spell*, MWWorld::TimeStamp> mUsedPowers;

            mutable bool mSpellsChanged;
            mutable MagicEffects mEffects;
            mutable std::map<const ESM::Spell*, MagicEffects> mSourcedEffects;
            void rebuildEffects() const;

            bool hasDisease(const ESM::Spell::SpellType type) const;

            using SpellFilter = bool (*)(const ESM::Spell*);
            void purge(const SpellFilter& filter);

            void addSpell(const ESM::Spell* spell);
            void removeSpell(const ESM::Spell* spell);
            void removeAllSpells();

            friend class SpellList;
        public:
            using TIterator = std::map<const ESM::Spell*, SpellParams>::const_iterator;

            Spells();

            Spells(const Spells&);

            Spells(Spells&& spells);

            ~Spells();

            static bool hasCorprusEffect(const ESM::Spell *spell);

            void purgeEffect(int effectId);
            void purgeEffect(int effectId, const std::string & sourceId);

            bool canUsePower (const ESM::Spell* spell) const;
            void usePower (const ESM::Spell* spell);

            void purgeCommonDisease();
            void purgeBlightDisease();
            void purgeCorprusDisease();
            void purgeCurses();

            TIterator begin() const;

            TIterator end() const;

            bool hasSpell(const std::string& spell) const;
            bool hasSpell(const ESM::Spell* spell) const;

            void add (const std::string& spell);
            ///< Adding a spell that is already listed in *this is a no-op.

            void add (const ESM::Spell* spell);
            ///< Adding a spell that is already listed in *this is a no-op.

            void remove (const std::string& spell);
            ///< If the spell to be removed is the selected spell, the selected spell will be changed to
            /// no spell (empty string).

            MagicEffects getMagicEffects() const;
            ///< Return sum of magic effects resulting from abilities, blights, deseases and curses.

            void clear(bool modifyBase = false);
            ///< Remove all spells of al types.

            void setSelectedSpell (const std::string& spellId);
            ///< This function does not verify, if the spell is available.

            const std::string getSelectedSpell() const;
            ///< May return an empty string.

            bool isSpellActive(const std::string& id) const;
            ///< Are we under the effects of the given spell ID?

            bool hasCommonDisease() const;

            bool hasBlightDisease() const;

            void removeEffects(const std::string& id);

            void visitEffectSources (MWMechanics::EffectSourceVisitor& visitor) const;

            void readState (const ESM::SpellState& state, CreatureStats* creatureStats);
            void writeState (ESM::SpellState& state) const;

            bool setSpells(const std::string& id);

            void addAllToInstance(const std::vector<std::string>& spells);
    };
}

#endif
