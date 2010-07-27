#ifndef GAME_MWMECHANICS_MECHANICSMANAGER_H
#define GAME_MWMECHANICS_MECHANICSMANAGER_H

#include <set>

#include "../mwworld/ptr.hpp"

namespace ESMS
{
    class ESMStore;
}

namespace MWGui
{
    class WindowManager;
}

namespace MWMechanics
{
    class MechanicsManager
    {
            const ESMS::ESMStore& mStore;
            MWGui::WindowManager& mWindowManager;
            std::set<MWWorld::Ptr> mActors;
    
        public:
        
            MechanicsManager (const ESMS::ESMStore& store, MWGui::WindowManager& windowManager);
            
            void configureGUI();
            
            void addActor (const MWWorld::Ptr& ptr);
            ///< Register an actor for stats management
            
            void dropActors (const MWWorld::Ptr::CellStore *cellStore);
            ///< Deregister all actors in the given cell.
    };
}

#endif

