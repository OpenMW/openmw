#include "controllerbuttonsoverlay.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

namespace MWGui
{
    ControllerButtonsOverlay::ControllerButtonsOverlay()
        : WindowBase("openmw_controllerbuttons.layout")
    {
        getWidget(mImageA, "BtnAImage");
        getWidget(mTextA, "BtnAText");

        getWidget(mImageB, "BtnBImage");
        getWidget(mTextB, "BtnBText");

        getWidget(mImageDpad, "BtnDpadImage");
        getWidget(mTextDpad, "BtnDpadText");

        getWidget(mImageL1, "BtnL1Image");
        getWidget(mTextL1, "BtnL1Text");

        getWidget(mImageL2, "BtnL2Image");
        getWidget(mTextL2, "BtnL2Text");

        getWidget(mImageL3, "BtnL3Image");
        getWidget(mTextL3, "BtnL3Text");

        getWidget(mImageLStick, "BtnLStickImage");
        getWidget(mTextLStick, "BtnLStickText");

        getWidget(mImageMenu, "BtnMenuImage");
        getWidget(mTextMenu, "BtnMenuText");

        getWidget(mImageR1, "BtnR1Image");
        getWidget(mTextR1, "BtnR1Text");

        getWidget(mImageR2, "BtnR2Image");
        getWidget(mTextR2, "BtnR2Text");

        getWidget(mImageR3, "BtnR3Image");
        getWidget(mTextR3, "BtnR3Text");

        getWidget(mImageRStick, "BtnRStickImage");
        getWidget(mTextRStick, "BtnRStickText");

        getWidget(mImageView, "BtnViewImage");
        getWidget(mTextView, "BtnViewText");

        getWidget(mImageX, "BtnXImage");
        getWidget(mTextX, "BtnXText");

        getWidget(mImageY, "BtnYImage");
        getWidget(mTextY, "BtnYText");

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

    int ControllerButtonsOverlay::updateButton(
        MyGUI::TextBox* text, MyGUI::ImageBox* image, const std::string& buttonStr)
    {
        if (buttonStr.length() > 0)
        {
            image->setVisible(true);
            image->setUserString("Hidden", "false");

            text->setCaptionWithReplacing(buttonStr);
            text->setVisible(true);
            text->setUserString("Hidden", "false");
            text->setSize(text->getTextSize().width + 16, 48);
            return 1;
        }
        else
        {
            image->setVisible(false);
            image->setUserString("Hidden", "true");

            text->setVisible(false);
            text->setUserString("Hidden", "true");
            return 0;
        }
    }
}
