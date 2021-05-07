#ifndef GAME_MWMECHANICS_ACTIVESPELLS_H
#define GAME_MWMECHANICS_ACTIVESPELLS_H

#include <map>
#include <vector>
#include <string>

#include <components/esm/activespells.hpp>

#include "../mwworld/timestamp.hpp"

#include "magiceffects.hpp"

namespace MWMechanics
{
    /// \brief Lasting spell effects
    ///
    /// \note The name of this class is slightly misleading, since it also handels lasting potion
    /// effects.
    class ActiveSpells
    {
        public:

            typedef ESM::ActiveEffect ActiveEffect;

            struct ActiveSpellParams
            {
                std::vector<ActiveEffect> mEffects;
                MWWorld::TimeStamp mTimeStamp;
                std::string mDisplayName;

                // The caster that inflicted this spell on us
                int mCasterActorId;
            };

            typedef std::multimap<std::string, ActiveSpellParams > TContainer;
            typedef TContainer::const_iterator TIterator;

            void readState (const ESM::ActiveSpells& state);
            void writeState (ESM::ActiveSpells& state) const;

            TIterator begin() const;

            TIterator end() const;

            void update(float duration) const;

        private:

            mutable TContainer mSpells;
            mutable MagicEffects mEffects;
            mutable bool mSpellsChanged;

            void rebuildEffects() const;

            /// Add any effects that are in "from" and not in "addTo" to "addTo"
            void mergeEffects(std::vector<ActiveEffect>& addTo, const std::vector<ActiveEffect>& from);

            double timeToExpire (const TIterator& iterator) const;
            ///< Returns time (in in-game hours) until the spell pointed to by \a iterator
            /// expires.

            const TContainer& getActiveSpells() const;

        public:

            ActiveSpells();

            /// Add lasting effects
            ///
            /// \brief addSpell
            /// \param id ID for stacking purposes.
            /// \param stack If false, the spell is not added if one with the same ID exists already.
            /// \param effects
            /// \param displayName Name for display in magic menu.
            ///
            void addSpell (const std::string& id, bool stack, std::vector<ActiveEffect> effects,
                           const std::string& displayName, int casterActorId);

            /// Removes the active effects from this spell/potion/.. with \a id
            void removeEffects (const std::string& id);

            /// Remove all active effects with this effect id
            void purgeEffect (short effectId);

            /// Remove all active effects with this effect id and source id
            void purgeEffect (short effectId, const std::string& sourceId, int effectIndex=-1);

            /// Remove all active effects, if roll succeeds (for each effect)
            void purgeAll(float chance, bool spellOnly = false);

            /// Remove all effects with CASTER_LINKED flag that were cast by \a casterActorId
            void purge (int casterActorId);

            /// Remove all spells
            void clear();

            bool isSpellActive (const std::string& id) const;
            ///< case insensitive

            void purgeCorprusDisease();

            const MagicEffects& getMagicEffects() const;

            void visitEffectSources (MWMechanics::EffectSourceVisitor& visitor) const;

    };
}

#endif
