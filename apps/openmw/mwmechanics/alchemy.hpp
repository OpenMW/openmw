#ifndef GAME_MWMECHANICS_ALCHEMY_H
#define GAME_MWMECHANICS_ALCHEMY_H

#include <vector>

#include "../mwworld/ptr.hpp"

namespace MWMechanics
{
    /// \brief Potion creation via alchemy skill
    class Alchemy
    {
        public:
        
            typedef std::vector<MWWorld::Ptr> TToolsContainer;
            typedef TToolsContainer::const_iterator TToolsIterator;
    
            typedef std::vector<MWWorld::Ptr> TIngredientsContainer;
            typedef TIngredientsContainer::const_iterator TIngredientsIterator;
    
        private:
    
            MWWorld::Ptr mNpc;
            TToolsContainer mTools;
            TIngredientsContainer mIngredients;

        public:
        
            void setAlchemist (const MWWorld::Ptr& npc);
            ///< Set alchemist and configure alchemy setup accordingly. \a npc may be empty to indicate that
            /// there is no alchemist (alchemy session has ended).
            
            TToolsIterator beginTools() const;
            
            TToolsIterator endTools() const;
            
            TIngredientsIterator beginIngredients() const;
            
            TIngredientsIterator endIngredients() const;
            
            void clear();
            ///< Remove alchemist, tools and ingredients.
            
            int addIngredient (const MWWorld::Ptr& ingredient);
            ///< Add ingredient into the next free slot.
            ///
            /// \return Slot index or -1, if adding failed because of no free slot or the ingredient type being
            /// listed already.
            
            void removeIngredient (int index);
            ///< Remove ingredient from slot (calling this function on an empty slot is a no-op).
    };
}

#endif

