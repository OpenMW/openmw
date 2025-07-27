#ifndef MWGUI_CONTROLLERBUTTONSOVERLAY_H
#define MWGUI_CONTROLLERBUTTONSOVERLAY_H

#include <MyGUI_ImageBox.h>
#include <MyGUI_TextBox.h>

#include <components/widgets/box.hpp>

#include "windowbase.hpp"

namespace MWGui
{
    class ControllerButtonsOverlay : public WindowBase
    {
    public:
        ControllerButtonsOverlay();

        int getHeight();
        void setButtons(ControllerButtons* buttons);

    private:
        enum Button
        {
            Button_A = 0,
            Button_B,
            Button_Dpad,
            Button_L1,
            Button_L2,
            Button_L3,
            Button_LStick,
            Button_Menu,
            Button_R1,
            Button_R2,
            Button_R3,
            Button_RStick,
            Button_View,
            Button_X,
            Button_Y,
            Button_Max,
        };

        struct ButtonDetails
        {
            std::string mLayoutName;
            std::string mImagePath;
            MyGUI::ImageBox* mImage = nullptr;
            MyGUI::TextBox* mText = nullptr;
            Gui::HBox* mHBox = nullptr;
        };

        std::array<ButtonDetails, Button::Button_Max> mButtons;

        Gui::HBox* mHBox;

        void setIcon(MyGUI::ImageBox* image, const std::string& imagePath);
        int updateButton(Button button, const std::string& buttonStr);
    };
}

#endif
