#ifndef GAME_MWMECHANICS_ACTIVESPELLS_H
#define GAME_MWMECHANICS_ACTIVESPELLS_H

#include <map>
#include <vector>
#include <string>

#include "../mwworld/timestamp.hpp"

#include "magiceffects.hpp"

#include <components/esm/defs.hpp>

namespace ESM
{
    struct Spell;
    struct EffectList;
}

namespace MWWorld
{
    class Ptr;
}

namespace MWMechanics
{
    struct ActiveSpellParams
    {
        // Only apply effects of this range type
        ESM::RangeType mRange;

        // When the spell was added
        MWWorld::TimeStamp mTimeStamp;

        // Random factor for each effect
        std::vector<float> mRandom;

        // Display name, we need this for enchantments, which don't have a name - so you need to supply the
        // name of the item with the enchantment to addSpell
        std::string mName;
    };

    /// \brief Lasting spell effects
    ///
    /// \note The name of this class is slightly misleading, since it also handels lasting potion
    /// effects.
    class ActiveSpells
    {
        public:

            typedef std::multimap<std::string, ActiveSpellParams > TContainer;
            typedef TContainer::const_iterator TIterator;

        private:

            mutable TContainer mSpells; // spellId, (time of casting, relative magnitude)
            mutable MagicEffects mEffects;
            mutable bool mSpellsChanged;
            mutable MWWorld::TimeStamp mLastUpdate;

            void update() const;
            
            void rebuildEffects() const;

            std::pair<ESM::EffectList, std::pair<bool, bool> > getEffectList (const std::string& id) const;
            ///< @return (EffectList, (isIngredient, stacks))

        public:

            ActiveSpells();

            bool addSpell (const std::string& id, const MWWorld::Ptr& actor, ESM::RangeType range = ESM::RT_Self, const std::string& name = "");
            ///< Overwrites an existing spell with the same ID. If the spell does not have any
            /// non-instant effects, it is ignored.
            /// @param id
            /// @param actor
            /// @param range Only effects with range type \a range will be applied
            /// @param name Display name for enchantments, since they don't have a name in their record
            ///
            /// \return Has the spell been added?

            void removeSpell (const std::string& id);

            bool isSpellActive (std::string id) const;
            ///< case insensitive

            const MagicEffects& getMagicEffects() const;

            const TContainer& getActiveSpells() const;

            TIterator begin() const;

            TIterator end() const;

            double timeToExpire (const TIterator& iterator) const;
            ///< Returns time (in in-game hours) until the spell pointed to by \a iterator
            /// expires.
    };
}

#endif
