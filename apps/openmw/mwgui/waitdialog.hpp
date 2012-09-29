#ifndef MWGUI_WAIT_DIALOG_H
#define MWGUI_WAIT_DIALOG_H

#include "window_base.hpp"

namespace MWGui
{

    class WaitDialogProgressBar : public WindowBase
    {
    public:
        WaitDialogProgressBar(MWBase::WindowManager& parWindowManager);

        virtual void open();

        void setProgress(int cur, int total);

    protected:
        MyGUI::ProgressBar* mProgressBar;
        MyGUI::TextBox* mProgressText;
    };

    class WaitDialog : public WindowBase
    {
    public:
        WaitDialog(MWBase::WindowManager& parWindowManager);

        virtual void open();

        void onFrame(float dt);

        void bedActivated() { setCanRest(true); }

        bool getSleeping() { return mWaiting && mSleeping; }
        void wakeUp();

    protected:
        MyGUI::TextBox* mDateTimeText;
        MyGUI::TextBox* mRestText;
        MyGUI::TextBox* mHourText;
        MyGUI::ScrollBar* mHourSlider;
        MyGUI::Button* mUntilHealedButton;
        MyGUI::Button* mWaitButton;
        MyGUI::Button* mCancelButton;

        bool mWaiting;
        bool mSleeping;
        int mCurHour;
        int mHours;
        float mRemainingTime;

        WaitDialogProgressBar mProgressBar;

        void onUntilHealedButtonClicked(MyGUI::Widget* sender);
        void onWaitButtonClicked(MyGUI::Widget* sender);
        void onCancelButtonClicked(MyGUI::Widget* sender);
        void onHourSliderChangedPosition(MyGUI::ScrollBar* sender, size_t position);

        void setCanRest(bool canRest);

        void startWaiting();
        void stopWaiting();
    };

}

#endif
