#ifndef GAME_MWMECHANICS_SPELLLIST_H
#define GAME_MWMECHANICS_SPELLLIST_H

#include <vector>

#include <components/esm/refid.hpp>

namespace ESM
{
    struct Spell;
    struct SpellState;
}

namespace MWMechanics
{
    class Spells;

    /// Multiple instances of the same actor share the same spell list in Morrowind.
    /// The most obvious result of this is that adding a spell or ability to one instance adds it to all instances.
    /// @note The original game will only update visual effects associated with any added abilities for the originally
    /// targeted actor,
    ///       changing cells applies the update to all actors.
    /// Aside from sharing the same active spell list, changes made to this list are also written to the actor's base
    /// record. Interestingly, it is not just scripted changes that are persisted to the base record. Curing one
    /// instance's disease will cure all instances.
    /// @note The original game is inconsistent in persisting this example;
    ///       saving and loading the game might reapply the cured disease depending on which instance was cured.
    class SpellList
    {
        std::vector<Spells*> mListeners;
        std::vector<ESM::RefId> mSpells;
        ESM::RefId mId;
        const int mType;

    public:
        SpellList(ESM::RefId id, int type, bool autoCalc);

        void add(const ESM::Spell* spell);
        ///< Adding a spell that is already listed in *this is a no-op.

        void remove(const ESM::Spell* spell);

        void removeAll(const std::vector<ESM::RefId>& spells);

        void clear();
        ///< Remove all spells of all types.

        void addListener(Spells* spells);

        void removeListener(Spells* spells);

        void updateListener(Spells* before, Spells* after);

        const std::vector<ESM::RefId>& getSpells() const;

        void setAutoCalc(const std::vector<const ESM::Spell*> spells);
    };
}

#endif
