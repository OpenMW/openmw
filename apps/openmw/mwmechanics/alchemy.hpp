#ifndef GAME_MWMECHANICS_ALCHEMY_H
#define GAME_MWMECHANICS_ALCHEMY_H

#include <vector>

#include "../mwworld/ptr.hpp"

namespace MWMechanics
{
    /// \brief Potion creatin via alchemy skill
    class Alchemy
    {
        public:
        
            typedef std::vector<MWWorld::Ptr> TToolsContainer;
            typedef TToolsContainer::const_iterator TToolsIterator;
    
        private:
    
            MWWorld::Ptr mNpc;
            TToolsContainer mTools;

        public:
        
            void setAlchemist (const MWWorld::Ptr& npc);
            ///< Set alchemist and configure alchemy setup accordingly. \a npc may be empty to indicate that
            /// there is no alchemist (alchemy session has ended).
            
            TToolsIterator beginTools() const;
            
            TToolsIterator endTools() const;
    };
}

#endif

