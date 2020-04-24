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

            Alchemy();

            typedef std::vector<MWWorld::Ptr> TToolsContainer;
            typedef TToolsContainer::const_iterator TToolsIterator;

            typedef std::vector<MWWorld::Ptr> TIngredientsContainer;
            typedef TIngredientsContainer::const_iterator TIngredientsIterator;

            typedef std::vector<ESM::ENAMstruct> TEffectsContainer;
            typedef TEffectsContainer::const_iterator TEffectsIterator;

            enum Result
            {
                Result_Success,

                Result_NoMortarAndPestle,
                Result_LessThanTwoIngredients,
                Result_NoName,
                Result_NoEffects,
                Result_RandomFailure
            };

        private:

            MWWorld::Ptr mAlchemist;
            TToolsContainer mTools;
            TIngredientsContainer mIngredients;
            TEffectsContainer mEffects;
            int mValue;
            std::string mPotionName;

            void applyTools (int flags, float& value) const;

            void updateEffects();

            Result getReadyStatus() const;

            const ESM::Potion *getRecord(const ESM::Potion& toFind) const;
            ///< Try to find a potion record similar to \a toFind in the record store, or return 0 if not found
            /// \note Does not account for record ID, model or icon

            void removeIngredients();
            ///< Remove selected ingredients from alchemist's inventory, cleanup selected ingredients and
            /// update effect list accordingly.

            void addPotion (const std::string& name);
            ///< Add a potion to the alchemist's inventory.

            void increaseSkill();
            ///< Increase alchemist's skill.

            Result createSingle ();
            ///< Try to create a potion from the ingredients, place it in the inventory of the alchemist and
            /// adjust the skills of the alchemist accordingly.

            float getAlchemyFactor() const;

            int countIngredients() const;

            TEffectsIterator beginEffects() const;

            TEffectsIterator endEffects() const;

        public:
            int countPotionsToBrew() const;
            ///< calculates maximum amount of potions, which you can make from selected ingredients

            static bool knownEffect (unsigned int potionEffectIndex, const MWWorld::Ptr& npc);
            ///< Does npc have sufficient alchemy skill to know about this potion effect?

            void setAlchemist (const MWWorld::Ptr& npc);
            ///< Set alchemist and configure alchemy setup accordingly. \a npc may be empty to indicate that
            /// there is no alchemist (alchemy session has ended).

            TToolsIterator beginTools() const;
            ///< \attention Iterates over tool slots, not over tools. Some of the slots may be empty.

            TToolsIterator endTools() const;

            TIngredientsIterator beginIngredients() const;
            ///< \attention Iterates over ingredient slots, not over ingredients. Some of the slots may be empty.

            TIngredientsIterator endIngredients() const;

            void clear();
            ///< Remove alchemist, tools and ingredients.

            void setPotionName(const std::string& name);
            ///< Set name of potion to create

            std::set<EffectKey> listEffects() const;
            ///< List all effects shared by at least two ingredients.

            int addIngredient (const MWWorld::Ptr& ingredient);
            ///< Add ingredient into the next free slot.
            ///
            /// \return Slot index or -1, if adding failed because of no free slot or the ingredient type being
            /// listed already.

            void removeIngredient (int index);
            ///< Remove ingredient from slot (calling this function on an empty slot is a no-op).

            std::string suggestPotionName ();
            ///< Suggest a name for the potion, based on the current effects

            Result create (const std::string& name, int& count);
            ///< Try to create potions from the ingredients, place them in the inventory of the alchemist and
            /// adjust the skills of the alchemist accordingly.
            /// \param name must not be an empty string, or Result_NoName is returned

            static std::vector<std::string> effectsDescription (const MWWorld::ConstPtr &ptr, const int alchemySKill);
    };
}

#endif

