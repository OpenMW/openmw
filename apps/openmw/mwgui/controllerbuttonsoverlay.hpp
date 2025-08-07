#ifndef MWGUI_CONTROLLERBUTTONSOVERLAY_H
#define MWGUI_CONTROLLERBUTTONSOVERLAY_H

#include <array>

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

        enum InputType
        {
            InputType_Button,
            InputType_Axis
        };

        struct ButtonDefinition
        {
            Button mButton;
            const char* mName;
            InputType mInputType;
            union
            {
                SDL_GameControllerButton mButton;
                SDL_GameControllerAxis mAxis;
            } mId;
            std::string MWGui::ControllerButtons::*mField;
        };

    private:
        struct ButtonWidgets
        {
            MyGUI::ImageBox* mImage;
            MyGUI::TextBox* mText;
            Gui::HBox* mHBox;

            ButtonWidgets()
                : mImage(nullptr)
                , mText(nullptr)
                , mHBox(nullptr)
            {
            }
        };

        std::array<ButtonWidgets, Button::Button_Max> mButtons;
        Gui::HBox* mHBox;

        void setIcon(MyGUI::ImageBox* image, const std::string& imagePath);
        int updateButton(Button button, const std::string& buttonStr);
    };
}

#endif
