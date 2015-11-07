#ifndef GAME_MWMECHANICS_SPELLS_H
#define GAME_MWMECHANICS_SPELLS_H

#include <map>
#include <string>

#include <components/misc/stringops.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/timestamp.hpp"

#include "magiceffects.hpp"


namespace ESM
{
    struct Spell;

    struct SpellState;
}

namespace MWMechanics
{
    class MagicEffects;

    /// \brief Spell list
    ///
    /// This class manages known spells as well as abilities, powers and permanent negative effects like
    /// diseases. It also keeps track of used powers (which can only be used every 24h).
    class Spells
    {
        public:

            typedef std::map<std::string, std::map<const int, float> > TContainer; // ID, <effect index, normalised random magnitude>
            typedef TContainer::const_iterator TIterator;

            struct CorprusStats
            {
                static const int sWorseningPeriod = 24;

                int mWorsenings;
                MWWorld::TimeStamp mNextWorsening;
            };

        private:

            TContainer mSpells;

            // spell-tied effects that will be applied even after removing the spell (currently used to keep positive effects when corprus is removed)
            std::map<std::string, MagicEffects> mPermanentSpellEffects;

            // Note: this is the spell that's about to be cast, *not* the spell selected in the GUI (which may be different)
            std::string mSelectedSpell;

            std::map<std::string, MWWorld::TimeStamp> mUsedPowers;

            std::map<std::string, CorprusStats> mCorprusSpells;

        public:

            void worsenCorprus(const std::string &corpSpellId);
            static bool hasCorprusEffect(const ESM::Spell *spell);
            const std::map<std::string, CorprusStats> & getCorprusSpells() const;

            bool canUsePower (const std::string& power) const;
            void usePower (const std::string& power);

            void purgeCommonDisease();
            void purgeBlightDisease();
            void purgeCorprusDisease();
            void purgeCurses();

            TIterator begin() const;

            TIterator end() const;

            bool hasSpell(const std::string& spell) const { return mSpells.find(Misc::StringUtils::lowerCase(spell)) != mSpells.end(); }

            void add (const std::string& spell);
            ///< Adding a spell that is already listed in *this is a no-op.

            void add (const ESM::Spell* spell);
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

            bool isSpellActive(const std::string& id) const;
            ///< Are we under the effects of the given spell ID?

            bool hasCommonDisease() const;

            bool hasBlightDisease() const;

            void visitEffectSources (MWMechanics::EffectSourceVisitor& visitor) const;

            void readState (const ESM::SpellState& state);
            void writeState (ESM::SpellState& state) const;
    };
}

#endif
