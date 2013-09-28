#ifndef GAME_MWMECHANICS_ACTIVESPELLS_H
#define GAME_MWMECHANICS_ACTIVESPELLS_H

#include <map>
#include <vector>
#include <string>

#include "../mwworld/timestamp.hpp"

#include "magiceffects.hpp"

namespace ESM
{
    struct Spell;
    struct EffectList;
}

namespace MWMechanics
{
    /// \brief Lasting spell effects
    ///
    /// \note The name of this class is slightly misleading, since it also handels lasting potion
    /// effects.
    class ActiveSpells
    {
        public:

            typedef std::map<std::string, std::pair<MWWorld::TimeStamp, float> > TContainer;
            typedef TContainer::const_iterator TIterator;

        private:

            mutable TContainer mSpells; // spellId, (time of casting, relative magnitude)
            mutable MagicEffects mEffects;
            mutable bool mSpellsChanged;
            mutable MWWorld::TimeStamp mLastUpdate;

            void update() const;

            const ESM::EffectList& getEffectList (const std::string& id) const;

        public:

            ActiveSpells();

            bool addSpell (const std::string& id);
            ///< Overwrites an existing spell with the same ID. If the spell does not have any
            /// non-instant effects, it is ignored.
            ///
            /// \return Has the spell been added?

            void removeSpell (const std::string& id);

            const MagicEffects& getMagicEffects() const;

            TIterator begin() const;

            TIterator end() const;

            double timeToExpire (const TIterator& iterator) const;
            ///< Returns time (in in-game hours) until the spell pointed to by \a iterator
            /// expires.
    };
}

#endif
