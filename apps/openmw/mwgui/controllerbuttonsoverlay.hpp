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

        void setButtons(ControllerButtonStr* buttons);

    private:
        MyGUI::ImageBox* mImageA;
        MyGUI::TextBox* mTextA;

        MyGUI::ImageBox* mImageB;
        MyGUI::TextBox* mTextB;

        MyGUI::ImageBox* mImageDpad;
        MyGUI::TextBox* mTextDpad;

        MyGUI::ImageBox* mImageL1;
        MyGUI::TextBox* mTextL1;

        MyGUI::ImageBox* mImageL2;
        MyGUI::TextBox* mTextL2;

        MyGUI::ImageBox* mImageL3;
        MyGUI::TextBox* mTextL3;

        MyGUI::ImageBox* mImageLStick;
        MyGUI::TextBox* mTextLStick;

        MyGUI::ImageBox* mImageMenu;
        MyGUI::TextBox* mTextMenu;

        MyGUI::ImageBox* mImageR1;
        MyGUI::TextBox* mTextR1;

        MyGUI::ImageBox* mImageR2;
        MyGUI::TextBox* mTextR2;

        MyGUI::ImageBox* mImageR3;
        MyGUI::TextBox* mTextR3;

        MyGUI::ImageBox* mImageRStick;
        MyGUI::TextBox* mTextRStick;

        MyGUI::ImageBox* mImageView;
        MyGUI::TextBox* mTextView;

        MyGUI::ImageBox* mImageX;
        MyGUI::TextBox* mTextX;

        MyGUI::ImageBox* mImageY;
        MyGUI::TextBox* mTextY;

        Gui::HBox* mHBox;

        int updateButton(MyGUI::TextBox* text, MyGUI::ImageBox* image, const std::string& buttonStr);
    };
}

#endif
