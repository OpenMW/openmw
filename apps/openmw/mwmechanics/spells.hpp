#ifndef GAME_MWMECHANICS_SPELLS_H
#define GAME_MWMECHANICS_SPELLS_H

#include <map>
#include <string>

#include "../mwworld/ptr.hpp"

#include "magiceffects.hpp"

namespace ESM
{
    struct Spell;
}

namespace MWMechanics
{
    class MagicEffects;

    /// \brief Spell list
    ///
    /// This class manages known spells as well as abilities, powers and permanent negative effects like
    /// diseaes.
    class Spells
    {
        public:

            typedef std::map<std::string, std::vector<float> > TContainer; // ID, normalised magnitudes
            typedef TContainer::const_iterator TIterator;

        private:

            TContainer mSpells;
            std::string mSelectedSpell;

        public:

            void purgeCommonDisease();
            void purgeBlightDisease();
            void purgeCorprusDisease();
            void purgeCurses();

            TIterator begin() const;

            TIterator end() const;

            bool hasSpell(const std::string& spell) { return mSpells.find(Misc::StringUtils::lowerCase(spell)) != mSpells.end(); }

            void add (const std::string& spell);
            ///< Adding a spell that is already listed in *this is a no-op.

            void remove (const std::string& spell);
            ///< If the spell to be removed is the selected spell, the selected spell will be changed to
            /// no spell (empty string).

            MagicEffects getMagicEffects() const;
            ///< Return sum of magic effects resulting from abilities, blights, deseases and curses.

            void clear();
            ///< Remove all spells of al types.

            void setSelectedSpell (const std::string& spellId);
            ///< This function does not verify, if the spell is available.

            const std::string getSelectedSpell() const;
            ///< May return an empty string.

            bool hasCommonDisease() const;

            bool hasBlightDisease() const;

            void visitEffectSources (MWMechanics::EffectSourceVisitor& visitor) const;
    };
}

#endif
