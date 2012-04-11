#ifndef GAME_MWMECHANICS_SPELLS_H
#define GAME_MWMECHANICS_SPELLS_H

#include <components/esm/loadspel.hpp>

#include <vector>

namespace MWWorld
{
    struct Environment;
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

            typedef std::vector<const ESM::Spell *> TContainer;
            typedef TContainer::const_iterator TIterator;

        private:

            static const int sTypes = 6;

            std::vector<const ESM::Spell *> mSpells[sTypes];

            void addSpell (const ESM::Spell *, MagicEffects& effects) const;

        public:

            TIterator begin (ESM::Spell::SpellType type) const;

            TIterator end (ESM::Spell::SpellType type) const;

            void add (const std::string& spell, MWWorld::Environment& environment);
            /// \note Adding a spell that is already listed in *this is a no-op.

            void remove (const std::string& spell, MWWorld::Environment& environment);

            MagicEffects getMagicEffects (MWWorld::Environment& environment) const;
            ///< Return sum of magic effects resulting from abilities, blights, deseases and curses.

            void clear();
            ///< Remove all spells of al types.
    };
}

#endif
