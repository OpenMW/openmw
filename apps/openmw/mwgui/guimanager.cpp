
#include "guimanager.hpp"

#include <iostream>

namespace MWGui
{
    void GuiManager::enableWindow (GuiWindow window)
    {
        std::cout << "enable window: " << window << std::endl;
    }
    
    void GuiManager::showOneTimeDialogue (GuiOneTimeDialogue dialogue)
    {
        std::cout << "show one time dialogue: " << dialogue << std::endl;
    }
    
    void GuiManager::enableDialogue (GuiDialogue dialogue)
    {
        std::cout << "enable dialogue: " << dialogue << std::endl;
    }
    
    void GuiManager::showDialogue (GuiDialogue dialogue)
    {
        std::cout << "show dialogue: " << dialogue << std::endl;
    }
            
    bool GuiManager::isGuiActive() const
    {
        return false;
    }
}
