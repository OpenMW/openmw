#ifndef MWGUI_CONTROLLERBUTTONSOVERLAY_H
#define MWGUI_CONTROLLERBUTTONSOVERLAY_H

#include "windowbase.hpp"

namespace MWGui
{
    class ControllerButtonsOverlay : public WindowBase
    {
    public:
        ControllerButtonsOverlay();

        void setButtonStr(const std::string& buttonStr);

    private:
        MyGUI::TextBox* mButtonStr;
    };
}

#endif
