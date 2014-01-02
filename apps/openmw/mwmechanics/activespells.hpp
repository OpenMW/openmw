#ifndef GAME_MWMECHANICS_ACTIVESPELLS_H
#define GAME_MWMECHANICS_ACTIVESPELLS_H

#include <map>
#include <vector>
#include <string>

#include "../mwworld/timestamp.hpp"

#include "magiceffects.hpp"

#include <components/esm/defs.hpp>

namespace MWMechanics
{
    /// \brief Lasting spell effects
    ///
    /// \note The name of this class is slightly misleading, since it also handels lasting potion
    /// effects.
    class ActiveSpells
    {
        public:

            // Parameters of an effect concerning lasting effects.
            // Note we are not using ENAMstruct since the magnitude may be modified by magic resistance, etc.
            // It could also be a negative magnitude, in case of inversing an effect, e.g. Absorb spell causes damage on target, but heals the caster.
            struct Effect
            {
                float mMagnitude;
                EffectKey mKey;
                float mDuration;
            };

            struct ActiveSpellParams
            {
                std::vector<Effect> mEffects;
                MWWorld::TimeStamp mTimeStamp;
                std::string mDisplayName;

                // Handle to the caster that that inflicted this spell on us
                std::string mCasterHandle;
            };

            typedef std::multimap<std::string, ActiveSpellParams > TContainer;
            typedef TContainer::const_iterator TIterator;

        private:

            mutable TContainer mSpells;
            mutable MagicEffects mEffects;
            mutable bool mSpellsChanged;
            mutable MWWorld::TimeStamp mLastUpdate;

            void update() const;
            
            void rebuildEffects() const;

            double timeToExpire (const TIterator& iterator) const;
            ///< Returns time (in in-game hours) until the spell pointed to by \a iterator
            /// expires.

            const TContainer& getActiveSpells() const;

            TIterator begin() const;

            TIterator end() const;

        public:

            ActiveSpells();

            /// Add lasting effects
            ///
            /// \brief addSpell
            /// \param id ID for stacking purposes.
            /// \param stack If false, the spell is not added if one with the same ID exists already.
            /// \param effects
            /// \param displayName Name for display in magic menu.
            /// \param casterHandle
            ///
            void addSpell (const std::string& id, bool stack, std::vector<Effect> effects,
                           const std::string& displayName, const std::string& casterHandle);

            /// Remove all active effects with this id
            void purgeEffect (short effectId);

            /// Remove all active effects, if roll succeeds (for each effect)
            void purgeAll (float chance);

            bool isSpellActive (std::string id) const;
            ///< case insensitive

            const MagicEffects& getMagicEffects() const;

            void visitEffectSources (MWMechanics::EffectSourceVisitor& visitor) const;

    };
}

#endif
