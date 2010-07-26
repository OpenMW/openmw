#ifndef GAME_MWMECHANICS_PTR_H
#define GAME_MWMECHANICS_PTR_H

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
    
        public:
        
            MechanicsManager (const ESMS::ESMStore& store, MWGui::WindowManager& windowManager);
            
            void configureGUI();
    };
}

#endif

