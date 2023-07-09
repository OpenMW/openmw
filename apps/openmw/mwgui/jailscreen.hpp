#ifndef MWGUI_JAILSCREEN_H
#define MWGUI_JAILSCREEN_H

#include "timeadvancer.hpp"
#include "windowbase.hpp"

namespace MWGui
{
    class JailScreen : public WindowBase
    {
    public:
        JailScreen();
        void goToJail(int days);

        void onFrame(float dt) override;

        bool exit() override { return false; }

        std::string_view getWindowIdForLua() const override { return "JailScreen"; }

    private:
        int mDays;

        float mFadeTimeRemaining;

        MyGUI::ScrollBar* mProgressBar;

        void onJailProgressChanged(int cur, int total);
        void onJailFinished();

        TimeAdvancer mTimeAdvancer;
    };
}

#endif
