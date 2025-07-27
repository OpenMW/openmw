#include "controllerbuttonsoverlay.hpp"

#include <MyGUI_Window.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/inputmanager.hpp"
#include "../mwbase/windowmanager.hpp"

namespace MWGui
{
    ControllerButtonsOverlay::ControllerButtonsOverlay()
        : WindowBase("openmw_controllerbuttons.layout")
    {
        MWBase::InputManager* inputMgr = MWBase::Environment::get().getInputManager();

        mButtons[Button::Button_A] = { "A", inputMgr->getControllerButtonIcon(SDL_CONTROLLER_BUTTON_A) };
        mButtons[Button::Button_B] = { "B", inputMgr->getControllerButtonIcon(SDL_CONTROLLER_BUTTON_B) };
        mButtons[Button::Button_Dpad] = { "Dpad", inputMgr->getControllerButtonIcon(SDL_CONTROLLER_BUTTON_DPAD_UP) };
        mButtons[Button::Button_L1] = { "L1", inputMgr->getControllerButtonIcon(SDL_CONTROLLER_BUTTON_LEFTSHOULDER) };
        mButtons[Button::Button_L2] = { "L2", inputMgr->getControllerAxisIcon(SDL_CONTROLLER_AXIS_TRIGGERLEFT) };
        mButtons[Button::Button_L3] = { "L3", inputMgr->getControllerButtonIcon(SDL_CONTROLLER_BUTTON_LEFTSTICK) };
        mButtons[Button::Button_LStick] = { "LStick", inputMgr->getControllerAxisIcon(SDL_CONTROLLER_AXIS_LEFTY) };
        mButtons[Button::Button_Menu] = { "Menu", inputMgr->getControllerButtonIcon(SDL_CONTROLLER_BUTTON_BACK) };
        mButtons[Button::Button_R1] = { "R1", inputMgr->getControllerButtonIcon(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) };
        mButtons[Button::Button_R2] = { "R2", inputMgr->getControllerAxisIcon(SDL_CONTROLLER_AXIS_TRIGGERRIGHT) };
        mButtons[Button::Button_R3] = { "R3", inputMgr->getControllerButtonIcon(SDL_CONTROLLER_BUTTON_RIGHTSTICK) };
        mButtons[Button::Button_RStick] = { "RStick", inputMgr->getControllerAxisIcon(SDL_CONTROLLER_AXIS_RIGHTY) };
        mButtons[Button::Button_View] = { "View", inputMgr->getControllerButtonIcon(SDL_CONTROLLER_BUTTON_START) };
        mButtons[Button::Button_X] = { "X", inputMgr->getControllerButtonIcon(SDL_CONTROLLER_BUTTON_X) };
        mButtons[Button::Button_Y] = { "Y", inputMgr->getControllerButtonIcon(SDL_CONTROLLER_BUTTON_Y) };

        for (size_t i = 0; i < mButtons.size(); i++)
        {
            getWidget(mButtons[i].mImage, "Btn" + mButtons[i].mLayoutName + "Image");
            getWidget(mButtons[i].mText, "Btn" + mButtons[i].mLayoutName + "Text");
            getWidget(mButtons[i].mHBox, "Btn" + mButtons[i].mLayoutName);
            setIcon(mButtons[i].mImage, mButtons[i].mImagePath);
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
            buttonCount += updateButton(Button::Button_A, buttons->mA);
            buttonCount += updateButton(Button::Button_B, buttons->mB);
            buttonCount += updateButton(Button::Button_Dpad, buttons->mDpad);
            buttonCount += updateButton(Button::Button_L1, buttons->mL1);
            buttonCount += updateButton(Button::Button_L2, buttons->mL2);
            buttonCount += updateButton(Button::Button_L3, buttons->mL3);
            buttonCount += updateButton(Button::Button_LStick, buttons->mLStick);
            buttonCount += updateButton(Button::Button_Menu, buttons->mMenu);
            buttonCount += updateButton(Button::Button_R1, buttons->mR1);
            buttonCount += updateButton(Button::Button_R2, buttons->mR2);
            buttonCount += updateButton(Button::Button_R3, buttons->mR3);
            buttonCount += updateButton(Button::Button_RStick, buttons->mRStick);
            buttonCount += updateButton(Button::Button_View, buttons->mView);
            buttonCount += updateButton(Button::Button_X, buttons->mX);
            buttonCount += updateButton(Button::Button_Y, buttons->mY);

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
