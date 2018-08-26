#ifndef GAME_MWMECHANICS_SPELLS_H
#define GAME_MWMECHANICS_SPELLS_H

#include <map>
#include <string>
#include <set>

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

            typedef const ESM::Spell* SpellKey;
            struct SpellParams {
                std::map<int, float> mEffectRands; // <effect index, normalised random magnitude>
                std::set<int> mPurgedEffects; // indices of purged effects
            };

            typedef std::map<SpellKey, SpellParams> TContainer;
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
            std::map<SpellKey, MagicEffects> mPermanentSpellEffects;

            // Note: this is the spell that's about to be cast, *not* the spell selected in the GUI (which may be different)
            std::string mSelectedSpell;

            std::map<SpellKey, MWWorld::TimeStamp> mUsedPowers;

            std::map<SpellKey, CorprusStats> mCorprusSpells;

            mutable bool mSpellsChanged;
            mutable MagicEffects mEffects;
            mutable std::map<SpellKey, MagicEffects> mSourcedEffects;
            void rebuildEffects() const;

            /// Get spell from ID, throws exception if not found
            const ESM::Spell* getSpell(const std::string& id) const;

        public:
            Spells();

            void worsenCorprus(const ESM::Spell* spell);
            static bool hasCorprusEffect(const ESM::Spell *spell);
            const std::map<SpellKey, CorprusStats> & getCorprusSpells() const;

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

            void removeEffects(const std::string& id);

            void visitEffectSources (MWMechanics::EffectSourceVisitor& visitor) const;

            void readState (const ESM::SpellState& state);
            void writeState (ESM::SpellState& state) const;
    };
}

#endif
