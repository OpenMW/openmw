
#include "mechanicsmanager.hpp"

#include <components/esm_store/store.hpp>

#include "../mwgui/window_manager.hpp"

namespace MWMechanics
{           
    MechanicsManager::MechanicsManager (const ESMS::ESMStore& store,
        MWGui::WindowManager& windowManager)
    : mStore (store), mWindowManager (windowManager)
    {
    
    }

    void MechanicsManager::configureGUI()
    {
        const char *names[][2] =
        {
            { "Attrib1", "sAttributeStrength" },
            { "Attrib2", "sAttributeIntelligence" },
            { "Attrib3", "sAttributeWillpower" },
            { "Attrib4", "sAttributeAgility" },
            { "Attrib5", "sAttributeSpeed" },
            { "Attrib6", "sAttributeEndurance" },
            { "Attrib7", "sAttributePersonality" },
            { "Attrib8", "sAttributeLuck" },
            { 0, 0 }
        };
    
        for (int i=0; names[i][0]; ++i)
        {
// This crashes because of encoding problems:          
//            std::string label = mStore.gameSettings.find (names[i][1])->str;

            std::string label = names[i][1]; // until the problem is fixed, use the GMST ID as label
            
            mWindowManager.setLabel (names[i][0], label);
        }
    }
    
    void MechanicsManager::addActor (const MWWorld::Ptr& ptr)
    {
        mActors.insert (ptr);
    }
    
    void MechanicsManager::removeActor (const MWWorld::Ptr& ptr)
    {
        mActors.erase (ptr);
    }
    
    void MechanicsManager::dropActors (const MWWorld::Ptr::CellStore *cellStore)
    {
        std::set<MWWorld::Ptr>::iterator iter = mActors.begin();
        
        while (iter!=mActors.end())
            if (iter->getCell()==cellStore)
            {
                mActors.erase (iter++);
            }
            else
                ++iter;
    }
    
    void MechanicsManager::update()
    {
    
    }
}

