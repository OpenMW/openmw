#ifndef MWGUI_WAIT_DIALOG_H
#define MWGUI_WAIT_DIALOG_H

#include "windowbase.hpp"
#include "widgets.hpp"

namespace MWGui
{

    class WaitDialogProgressBar : public WindowBase
    {
    public:
        WaitDialogProgressBar();

        virtual void open();

        void setProgress(int cur, int total);

    protected:
        MyGUI::ProgressBar* mProgressBar;
        MyGUI::TextBox* mProgressText;
    };

    class WaitDialog : public WindowBase
    {
    public:
        WaitDialog();

        virtual void open();

        virtual void exit();

        void onFrame(float dt);

        void bedActivated() { setCanRest(true); }

        bool getSleeping() { return mWaiting && mSleeping; }
        void wakeUp();
        void autosave();

    protected:
        MyGUI::TextBox* mDateTimeText;
        MyGUI::TextBox* mRestText;
        MyGUI::TextBox* mHourText;
        MyGUI::Button* mUntilHealedButton;
        MyGUI::Button* mWaitButton;
        MyGUI::Button* mCancelButton;
        MWGui::Widgets::MWScrollBar* mHourSlider;

        bool mWaiting;
        bool mSleeping;
        int mCurHour;
        int mHours;
        int mManualHours; // stores the hours to rest selected via slider
        float mRemainingTime;

        int mInterruptAt;
        std::string mInterruptCreatureList;

        WaitDialogProgressBar mProgressBar;

        void onUntilHealedButtonClicked(MyGUI::Widget* sender);
        void onWaitButtonClicked(MyGUI::Widget* sender);
        void onCancelButtonClicked(MyGUI::Widget* sender);
        void onHourSliderChangedPosition(MyGUI::ScrollBar* sender, size_t position);

        void setCanRest(bool canRest);

        void startWaiting(int hoursToWait);
        void stopWaiting();
    };

}

#endif
