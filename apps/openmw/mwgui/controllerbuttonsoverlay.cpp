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
            getWidget(mButtons[i].mImage, "Btn" + mButtons[i].mName + "Image");
            getWidget(mButtons[i].mText, "Btn" + mButtons[i].mName + "Text");
            setIcon(mButtons[i].mImage, mButtons[i].mImagePath);
        }

        getWidget(mHBox, "ButtonBox");
    }

    int ControllerButtonsOverlay::getHeight()
    {
        MyGUI::Window* window = mMainWidget->castType<MyGUI::Window>();
        return window->getHeight();
    }

    void ControllerButtonsOverlay::setButtons(ControllerButtonStr* buttons)
    {
        int buttonCount = 0;
        buttonCount += updateButton(Button::Button_A, buttons->a);
        buttonCount += updateButton(Button::Button_B, buttons->b);
        buttonCount += updateButton(Button::Button_Dpad, buttons->dpad);
        buttonCount += updateButton(Button::Button_L1, buttons->l1);
        buttonCount += updateButton(Button::Button_L2, buttons->l2);
        buttonCount += updateButton(Button::Button_L3, buttons->l3);
        buttonCount += updateButton(Button::Button_LStick, buttons->lStick);
        buttonCount += updateButton(Button::Button_Menu, buttons->menu);
        buttonCount += updateButton(Button::Button_R1, buttons->r1);
        buttonCount += updateButton(Button::Button_R2, buttons->r2);
        buttonCount += updateButton(Button::Button_R3, buttons->r3);
        buttonCount += updateButton(Button::Button_RStick, buttons->rStick);
        buttonCount += updateButton(Button::Button_View, buttons->view);
        buttonCount += updateButton(Button::Button_X, buttons->x);
        buttonCount += updateButton(Button::Button_Y, buttons->y);

        mHBox->notifyChildrenSizeChanged();

        setVisible(buttonCount > 0);
    }

    void ControllerButtonsOverlay::setIcon(MyGUI::ImageBox* image, const std::string& imagePath)
    {
        if (!imagePath.empty())
            image->setImageTexture(imagePath);
    }

    int ControllerButtonsOverlay::updateButton(ControllerButtonsOverlay::Button button, const std::string& buttonStr)
    {
        MyGUI::TextBox* text = mButtons[button].mText;
        MyGUI::ImageBox* image = mButtons[button].mImage;

        if (buttonStr.empty())
        {
            image->setVisible(false);
            image->setUserString("Hidden", "true");

            text->setVisible(false);
            text->setUserString("Hidden", "true");
            return 0;
        }
        else
        {
            image->setVisible(true);
            image->setUserString("Hidden", "false");

            text->setCaptionWithReplacing(buttonStr);
            text->setVisible(true);
            text->setUserString("Hidden", "false");
            text->setSize(text->getTextSize().width + 16, 48);
            return 1;
        }
    }
}
