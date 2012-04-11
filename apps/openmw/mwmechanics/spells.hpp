#ifndef GAME_MWMECHANICS_SPELLS_H
#define GAME_MWMECHANICS_SPELLS_H

#include <vector>
#include <string>

namespace ESM
{
    struct Spell;
}

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

            typedef std::vector<std::string> TContainer;
            typedef TContainer::const_iterator TIterator;

        private:

            std::vector<std::string> mSpells;

            void addSpell (const ESM::Spell *, MagicEffects& effects) const;

        public:

            TIterator begin() const;

            TIterator end() const;

            void add (const std::string& spell);
            /// \note Adding a spell that is already listed in *this is a no-op.

            void remove (const std::string& spell);

            MagicEffects getMagicEffects (const MWWorld::Environment& environment) const;
            ///< Return sum of magic effects resulting from abilities, blights, deseases and curses.

            void clear();
            ///< Remove all spells of al types.
    };
}

#endif
