#include "controllerbuttonsoverlay.hpp"

#include <MyGUI_Window.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/inputmanager.hpp"
#include "../mwbase/windowmanager.hpp"

namespace MWGui
{
    static constexpr ControllerButtonsOverlay::ButtonDefinition sButtonDefs[] = {
        { ControllerButtonsOverlay::Button::Button_A, "A", SDL_CONTROLLER_BUTTON_A, &ControllerButtons::mA },
        { ControllerButtonsOverlay::Button::Button_B, "B", SDL_CONTROLLER_BUTTON_B, &ControllerButtons::mB },
        { ControllerButtonsOverlay::Button::Button_Dpad, "Dpad", SDL_CONTROLLER_BUTTON_DPAD_UP,
            &ControllerButtons::mDpad },
        { ControllerButtonsOverlay::Button::Button_L1, "L1", SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
            &ControllerButtons::mL1 },
        { ControllerButtonsOverlay::Button::Button_L2, "L2", SDL_CONTROLLER_AXIS_TRIGGERLEFT, &ControllerButtons::mL2 },
        { ControllerButtonsOverlay::Button::Button_L3, "L3", SDL_CONTROLLER_BUTTON_LEFTSTICK, &ControllerButtons::mL3 },
        { ControllerButtonsOverlay::Button::Button_LStick, "LStick", SDL_CONTROLLER_AXIS_LEFTY,
            &ControllerButtons::mLStick },
        { ControllerButtonsOverlay::Button::Button_Menu, "Menu", SDL_CONTROLLER_BUTTON_BACK,
            &ControllerButtons::mMenu },
        { ControllerButtonsOverlay::Button::Button_R1, "R1", SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
            &ControllerButtons::mR1 },
        { ControllerButtonsOverlay::Button::Button_R2, "R2", SDL_CONTROLLER_AXIS_TRIGGERRIGHT,
            &ControllerButtons::mR2 },
        { ControllerButtonsOverlay::Button::Button_R3, "R3", SDL_CONTROLLER_BUTTON_RIGHTSTICK,
            &ControllerButtons::mR3 },
        { ControllerButtonsOverlay::Button::Button_RStick, "RStick", SDL_CONTROLLER_AXIS_RIGHTY,
            &ControllerButtons::mRStick },
        { ControllerButtonsOverlay::Button::Button_View, "View", SDL_CONTROLLER_BUTTON_START,
            &ControllerButtons::mView },
        { ControllerButtonsOverlay::Button::Button_X, "X", SDL_CONTROLLER_BUTTON_X, &ControllerButtons::mX },
        { ControllerButtonsOverlay::Button::Button_Y, "Y", SDL_CONTROLLER_BUTTON_Y, &ControllerButtons::mY },
    };

    ControllerButtonsOverlay::ControllerButtonsOverlay()
        : WindowBase("openmw_controllerbuttons.layout")
    {
        MWBase::InputManager* inputMgr = MWBase::Environment::get().getInputManager();

        for (size_t i = 0; i < mButtons.size(); i++)
        {
            getWidget(mButtons[i].mImage, "Btn" + sButtonDefs[i].mName + "Image");
            getWidget(mButtons[i].mText, "Btn" + sButtonDefs[i].mName + "Text");
            getWidget(mButtons[i].mHBox, "Btn" + sButtonDefs[i].mName);
            if (std::holds_alternative<SDL_GameControllerAxis>(sButtonDefs[i].mId))
                setIcon(mButtons[i].mImage,
                    inputMgr->getControllerAxisIcon(std::get<SDL_GameControllerAxis>(sButtonDefs[i].mId)));
            else
                setIcon(mButtons[i].mImage,
                    inputMgr->getControllerButtonIcon(std::get<SDL_GameControllerButton>(sButtonDefs[i].mId)));
        }

        getWidget(mHBox, "ButtonBox");
    }

    int ControllerButtonsOverlay::getHeight()
    {
        MyGUI::Window* window = mMainWidget->castType<MyGUI::Window>();
        return window->getHeight();
    }

    void ControllerButtonsOverlay::setButtons(ControllerButtons* buttons)
    {
        int buttonCount = 0;
        if (buttons != nullptr)
        {
            for (const auto& row : sButtonDefs)
                buttonCount += updateButton(row.mButton, buttons->*(row.mField));

            mHBox->notifyChildrenSizeChanged();
        }

        setVisible(buttonCount > 0);
    }

    void ControllerButtonsOverlay::setIcon(MyGUI::ImageBox* image, const std::string& imagePath)
    {
        if (!imagePath.empty())
            image->setImageTexture(imagePath);
    }

    int ControllerButtonsOverlay::updateButton(ControllerButtonsOverlay::Button button, const std::string& buttonStr)
    {
        if (buttonStr.empty())
        {
            mButtons[button].mHBox->setVisible(false);
            mButtons[button].mHBox->setUserString("Hidden", "true");
            return 0;
        }
        else
        {
            mButtons[button].mHBox->setVisible(true);
            mButtons[button].mHBox->setUserString("Hidden", "false");
            mButtons[button].mText->setCaptionWithReplacing(buttonStr);
            return 1;
        }
    }
}
