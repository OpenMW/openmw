#ifndef GAME_MWMECHANICS_ALCHEMY_H
#define GAME_MWMECHANICS_ALCHEMY_H

#include <vector>
#include <set>

#include <components/esm/effectlist.hpp>

#include "../mwworld/ptr.hpp"

namespace ESM
{
    struct Potion;
}

namespace MWMechanics
{
    struct EffectKey;

    /// \brief Potion creation via alchemy skill
    class Alchemy
    {
        public:

            typedef std::vector<MWWorld::Ptr> TToolsContainer;
            typedef TToolsContainer::const_iterator TToolsIterator;

            typedef std::vector<MWWorld::Ptr> TIngredientsContainer;
            typedef TIngredientsContainer::const_iterator TIngredientsIterator;

            typedef std::vector<ESM::ENAMstruct> TEffectsContainer;
            typedef TEffectsContainer::const_iterator TEffectsIterator;

            /// Result of potion creation
            /** Only Result_Success results in success **/
            enum Result
            {
                Result_Success,

                Result_NoMortarAndPestle,
                Result_LessThanTwoIngredients,
                Result_NoName,
                Result_NoEffects,
                Result_RandomFailure
            };

            /// Set alchemist and configure alchemy setup accordingly.
            /** \a npc may be empty to indicate that there is no alchemist (alchemy session has ended). **/
            void setAlchemist (const MWWorld::Ptr& npc);

            /// \attention Iterates over tool slots, not over tools. Some of the slots may be empty.
            TToolsIterator beginTools() const;

            TToolsIterator endTools() const;

            /// \attention Iterates over ingredient slots, not over ingredients. Some of the slots may be empty.
            TIngredientsIterator beginIngredients() const;

            TIngredientsIterator endIngredients() const;

            /// Remove alchemist, tools and ingredients.
            void clear();

            /// Add ingredient into the next free slot.
            ///
            /// \return Slot index or -1, if adding failed because of no free slot or the ingredient type being
            /// listed already.
            int addIngredient (const MWWorld::Ptr& ingredient);

            /// Remove ingredient from slot (calling this function on an empty slot is a no-op).
            void removeIngredient (int index);

            TEffectsIterator beginEffects() const;

            TEffectsIterator endEffects() const;

            /// Return the name of the potion that would be created when calling create (if a record for such
            /// a potion already exists) or return an empty string.
            std::string getPotionName() const;

            /// Try to create a potion from the ingredients, place it in the inventory of the alchemist and
            /// adjust the skills of the alchemist accordingly.
            /// \param name must not be an empty string, unless there is already a potion record (
            /// getPotionName() does not return an empty string).
            Result create (const std::string& name);

        private:

            MWWorld::Ptr mAlchemist;
            TToolsContainer mTools;
            TIngredientsContainer mIngredients;
            TEffectsContainer mEffects;
            int mValue;

            /// List all effects shared by at least two ingredients.
            std::set<EffectKey> listEffects() const;

            void applyTools (int flags, float& value) const;

            void updateEffects();

            /// Return existing recrod for created potion (may return 0)
            const ESM::Potion *getRecord() const;

            /// Remove selected ingredients from alchemist's inventory, cleanup selected ingredients and
            /// update effect list accordingly.
            void removeIngredients();

            void addPotion (const std::string& name);
            ///< Add a potion to the alchemist's inventory.

            void increaseSkill();
            ///< Increase alchemist's skill.

            float getChance() const;
            ///< Return chance of success.

            int countIngredients() const;
    };
}

#endif

