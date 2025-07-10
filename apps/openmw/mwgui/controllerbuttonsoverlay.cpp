#include "controllerbuttonsoverlay.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/inputmanager.hpp"
#include "../mwbase/windowmanager.hpp"

namespace MWGui
{
    ControllerButtonsOverlay::ControllerButtonsOverlay()
        : WindowBase("openmw_controllerbuttons.layout")
    {
        MWBase::InputManager* inputMgr = MWBase::Environment::get().getInputManager();

        getWidget(mImageA, "BtnAImage");
        getWidget(mTextA, "BtnAText");
        setIcon(mImageA, inputMgr->getControllerButtonIcon(SDL_CONTROLLER_BUTTON_A));

        getWidget(mImageB, "BtnBImage");
        getWidget(mTextB, "BtnBText");
        setIcon(mImageB, inputMgr->getControllerButtonIcon(SDL_CONTROLLER_BUTTON_B));

        getWidget(mImageDpad, "BtnDpadImage");
        getWidget(mTextDpad, "BtnDpadText");
        setIcon(mImageDpad, inputMgr->getControllerButtonIcon(SDL_CONTROLLER_BUTTON_DPAD_UP));

        getWidget(mImageL1, "BtnL1Image");
        getWidget(mTextL1, "BtnL1Text");
        setIcon(mImageL1, inputMgr->getControllerButtonIcon(SDL_CONTROLLER_BUTTON_LEFTSHOULDER));

        getWidget(mImageL2, "BtnL2Image");
        getWidget(mTextL2, "BtnL2Text");
        setIcon(mImageL2, inputMgr->getControllerAxisIcon(SDL_CONTROLLER_AXIS_TRIGGERLEFT));

        getWidget(mImageL3, "BtnL3Image");
        getWidget(mTextL3, "BtnL3Text");
        setIcon(mImageL3, inputMgr->getControllerButtonIcon(SDL_CONTROLLER_BUTTON_LEFTSTICK));

        getWidget(mImageLStick, "BtnLStickImage");
        getWidget(mTextLStick, "BtnLStickText");
        setIcon(mImageLStick, inputMgr->getControllerAxisIcon(SDL_CONTROLLER_AXIS_LEFTY));

        getWidget(mImageMenu, "BtnMenuImage");
        getWidget(mTextMenu, "BtnMenuText");
        setIcon(mImageMenu, inputMgr->getControllerButtonIcon(SDL_CONTROLLER_BUTTON_BACK));

        getWidget(mImageR1, "BtnR1Image");
        getWidget(mTextR1, "BtnR1Text");
        setIcon(mImageR1, inputMgr->getControllerButtonIcon(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER));

        getWidget(mImageR2, "BtnR2Image");
        getWidget(mTextR2, "BtnR2Text");
        setIcon(mImageR2, inputMgr->getControllerAxisIcon(SDL_CONTROLLER_AXIS_TRIGGERRIGHT));

        getWidget(mImageR3, "BtnR3Image");
        getWidget(mTextR3, "BtnR3Text");
        setIcon(mImageR3, inputMgr->getControllerButtonIcon(SDL_CONTROLLER_BUTTON_RIGHTSTICK));

        getWidget(mImageRStick, "BtnRStickImage");
        getWidget(mTextRStick, "BtnRStickText");
        setIcon(mImageRStick, inputMgr->getControllerAxisIcon(SDL_CONTROLLER_AXIS_RIGHTY));

        getWidget(mImageView, "BtnViewImage");
        getWidget(mTextView, "BtnViewText");
        setIcon(mImageView, inputMgr->getControllerButtonIcon(SDL_CONTROLLER_BUTTON_START));

        getWidget(mImageX, "BtnXImage");
        getWidget(mTextX, "BtnXText");
        setIcon(mImageX, inputMgr->getControllerButtonIcon(SDL_CONTROLLER_BUTTON_X));

        getWidget(mImageY, "BtnYImage");
        getWidget(mTextY, "BtnYText");
        setIcon(mImageY, inputMgr->getControllerButtonIcon(SDL_CONTROLLER_BUTTON_Y));

        getWidget(mHBox, "ButtonBox");
    }

    void ControllerButtonsOverlay::setButtons(ControllerButtonStr* buttons)
    {
        int buttonCount = 0;
        buttonCount += updateButton(mTextA, mImageA, buttons->a);
        buttonCount += updateButton(mTextB, mImageB, buttons->b);
        buttonCount += updateButton(mTextDpad, mImageDpad, buttons->dpad);
        buttonCount += updateButton(mTextL1, mImageL1, buttons->l1);
        buttonCount += updateButton(mTextL2, mImageL2, buttons->l2);
        buttonCount += updateButton(mTextL3, mImageL3, buttons->l3);
        buttonCount += updateButton(mTextLStick, mImageLStick, buttons->lStick);
        buttonCount += updateButton(mTextMenu, mImageMenu, buttons->menu);
        buttonCount += updateButton(mTextR1, mImageR1, buttons->r1);
        buttonCount += updateButton(mTextR2, mImageR2, buttons->r2);
        buttonCount += updateButton(mTextR3, mImageR3, buttons->r3);
        buttonCount += updateButton(mTextRStick, mImageRStick, buttons->rStick);
        buttonCount += updateButton(mTextView, mImageView, buttons->view);
        buttonCount += updateButton(mTextX, mImageX, buttons->x);
        buttonCount += updateButton(mTextY, mImageY, buttons->y);
        mHBox->notifyChildrenSizeChanged();

        setVisible(buttonCount > 0);
    }

    void ControllerButtonsOverlay::setIcon(MyGUI::ImageBox* image, const std::string& imagePath)
    {
        if (!imagePath.empty())
            image->setImageTexture(imagePath);
    }

    int ControllerButtonsOverlay::updateButton(
        MyGUI::TextBox* text, MyGUI::ImageBox* image, const std::string& buttonStr)
    {
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
