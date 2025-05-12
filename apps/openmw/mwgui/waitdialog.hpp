#ifndef MWGUI_WAIT_DIALOG_H
#define MWGUI_WAIT_DIALOG_H

#include "timeadvancer.hpp"
#include "windowbase.hpp"
#include <components/esm/refid.hpp>
#include <components/widgets/scrollbar.hpp>

namespace MWGui
{

    class WaitDialogProgressBar : public WindowBase
    {
    public:
        WaitDialogProgressBar();

        void onOpen() override;

        void setProgress(int cur, int total);

    protected:
        MyGUI::ProgressBar* mProgressBar;
        MyGUI::TextBox* mProgressText;
    };

    class WaitDialog : public WindowBase
    {
    public:
        WaitDialog();

        void setPtr(const MWWorld::Ptr& ptr) override;

        void onOpen() override;

        bool exit() override;

        void clear() override;

        void onFrame(float dt) override;
        bool onControllerButtonEvent(const SDL_ControllerButtonEvent& arg) override;

        bool getSleeping() { return mTimeAdvancer.isRunning() && mSleeping; }
        void wakeUp();
        void autosave();

        WindowBase* getProgressBar() { return &mProgressBar; }

        std::string_view getWindowIdForLua() const override { return "WaitDialog"; }

    protected:
        MyGUI::TextBox* mDateTimeText;
        MyGUI::TextBox* mRestText;
        MyGUI::TextBox* mHourText;
        MyGUI::Button* mUntilHealedButton;
        MyGUI::Button* mWaitButton;
        MyGUI::Button* mCancelButton;
        Gui::ScrollBar* mHourSlider;

        TimeAdvancer mTimeAdvancer;
        bool mSleeping;
        int mHours;
        int mManualHours; // stores the hours to rest selected via slider
        float mFadeTimeRemaining;

        int mInterruptAt;
        ESM::RefId mInterruptCreatureList;

        WaitDialogProgressBar mProgressBar;

        void onUntilHealedButtonClicked(MyGUI::Widget* sender);
        void onWaitButtonClicked(MyGUI::Widget* sender);
        void onCancelButtonClicked(MyGUI::Widget* sender);
        void onHourSliderChangedPosition(MyGUI::ScrollBar* sender, size_t position);
        void onKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char character);

        void onWaitingProgressChanged(int cur, int total);
        void onWaitingInterrupted();
        void onWaitingFinished();

        void setCanRest(bool canRest);

        void startWaiting(int hoursToWait);
        void stopWaiting();
    };

}

#endif
