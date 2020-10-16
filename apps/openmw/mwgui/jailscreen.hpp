#ifndef MWGUI_JAILSCREEN_H
#define MWGUI_JAILSCREEN_H

#include "windowbase.hpp"
#include "timeadvancer.hpp"

namespace MWGui
{
    class JailScreen : public WindowBase
    {
        public:
            JailScreen();
            void goToJail(int days);

            void onFrame(float dt) override;

            bool exit() override { return false; }

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
