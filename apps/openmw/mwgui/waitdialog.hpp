#ifndef MWGUI_WAIT_DIALOG_H
#define MWGUI_WAIT_DIALOG_H

#include "window_base.hpp"

namespace MWGui
{

    class WaitDialog : public WindowBase
    {
    public:
        WaitDialog(MWBase::WindowManager& parWindowManager);

        virtual void open();

    protected:
        MyGUI::TextBox* mDateTimeText;
        MyGUI::TextBox* mRestText;
        MyGUI::TextBox* mHourText;
        MyGUI::ScrollBar* mHourSlider;
        MyGUI::Button* mUntilHealedButton;
        MyGUI::Button* mWaitButton;
        MyGUI::Button* mCancelButton;

        void onUntilHealedButtonClicked(MyGUI::Widget* sender);
        void onWaitButtonClicked(MyGUI::Widget* sender);
        void onCancelButtonClicked(MyGUI::Widget* sender);
        void onHourSliderChangedPosition(MyGUI::ScrollBar* sender, size_t position);

        void setCanRest(bool canRest);
    };

}

#endif
