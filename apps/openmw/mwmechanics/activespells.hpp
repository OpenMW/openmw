#ifndef GAME_MWMECHANICS_ACTIVESPELLS_H
#define GAME_MWMECHANICS_ACTIVESPELLS_H

#include <functional>
#include <list>
#include <queue>
#include <string>
#include <variant>
#include <vector>

#include <components/esm/activespells.hpp>

#include "../mwworld/timestamp.hpp"
#include "../mwworld/ptr.hpp"

#include "magiceffects.hpp"
#include "spellcasting.hpp"

namespace ESM
{
    struct Enchantment;
    struct Spell;
}

namespace MWMechanics
{
    /// \brief Lasting spell effects
    ///
    /// \note The name of this class is slightly misleading, since it also handles lasting potion
    /// effects.
    class ActiveSpells
    {
        public:

            using ActiveEffect = ESM::ActiveEffect;
            class ActiveSpellParams
            {
                    std::string mId;
                    std::vector<ActiveEffect> mEffects;
                    std::string mDisplayName;
                    int mCasterActorId;
                    int mSlot;
                    ESM::ActiveSpells::EffectType mType;
                    int mWorsenings;
                    MWWorld::TimeStamp mNextWorsening;

                    ActiveSpellParams(const ESM::ActiveSpells::ActiveSpellParams& params);

                    ActiveSpellParams(const ESM::Spell* spell, const MWWorld::Ptr& actor, bool ignoreResistances = false);

                    ActiveSpellParams(const MWWorld::ConstPtr& item, const ESM::Enchantment* enchantment, int slotIndex, const MWWorld::Ptr& actor);

                    ActiveSpellParams(const ActiveSpellParams& params, const MWWorld::Ptr& actor);

                    ESM::ActiveSpells::ActiveSpellParams toEsm() const;

                    friend class ActiveSpells;
                public:
                    ActiveSpellParams(const CastSpell& cast, const MWWorld::Ptr& caster);

                    const std::string& getId() const { return mId; }

                    const std::vector<ActiveEffect>& getEffects() const { return mEffects; }
                    std::vector<ActiveEffect>& getEffects() { return mEffects; }

                    ESM::ActiveSpells::EffectType getType() const { return mType; }

                    int getCasterActorId() const { return mCasterActorId; }

                    int getWorsenings() const { return mWorsenings; }

                    const std::string& getDisplayName() const { return mDisplayName; }

                    // Increments worsenings count and sets the next timestamp
                    void worsen();

                    bool shouldWorsen() const;

                    void resetWorsenings();
            };

            typedef std::list<ActiveSpellParams>::const_iterator TIterator;

            void readState (const ESM::ActiveSpells& state);
            void writeState (ESM::ActiveSpells& state) const;

            TIterator begin() const;

            TIterator end() const;

            void update(const MWWorld::Ptr& ptr, float duration);

        private:
            using ParamsPredicate = std::function<bool(const ActiveSpellParams&)>;
            using EffectPredicate = std::function<bool(const ActiveSpellParams&, const ESM::ActiveEffect&)>;
            using Predicate = std::variant<ParamsPredicate, EffectPredicate>;

            struct IterationGuard
            {
                ActiveSpells& mActiveSpells;

                IterationGuard(ActiveSpells& spells);
                ~IterationGuard();
            };

            std::list<ActiveSpellParams> mSpells;
            std::vector<ActiveSpellParams> mQueue;
            std::queue<Predicate> mPurges;
            bool mIterating;

            void addToSpells(const MWWorld::Ptr& ptr, const ActiveSpellParams& spell);

            bool applyPurges(const MWWorld::Ptr& ptr, std::list<ActiveSpellParams>::iterator* currentSpell = nullptr, std::vector<ActiveEffect>::iterator* currentEffect = nullptr);

        public:

            ActiveSpells();

            /// Add lasting effects
            ///
            /// \brief addSpell
            /// \param id ID for stacking purposes.
            ///
            void addSpell (const ActiveSpellParams& params);

            /// Bypasses resistances
            void addSpell(const ESM::Spell* spell, const MWWorld::Ptr& actor);

            /// Removes the active effects from this spell/potion/.. with \a id
            void removeEffects (const MWWorld::Ptr& ptr, const std::string& id);

            /// Remove all active effects with this effect id
            void purgeEffect (const MWWorld::Ptr& ptr, short effectId);

            void purge(EffectPredicate predicate, const MWWorld::Ptr& ptr);
            void purge(ParamsPredicate predicate, const MWWorld::Ptr& ptr);

            /// Remove all effects that were cast by \a casterActorId
            void purge (const MWWorld::Ptr& ptr, int casterActorId);

            /// Remove all spells
            void clear(const MWWorld::Ptr& ptr);

            bool isSpellActive (const std::string& id) const;
            ///< case insensitive

            void skipWorsenings(double hours);
    };
}

#endif
