#ifndef MWGUI_TIMEADVANCER_H
#define MWGUI_TIMEADVANCER_H

#include <MyGUI_Delegate.h>

namespace MWGui
{
    class TimeAdvancer
    {
    public:
        TimeAdvancer();

        void run(int hours, int interruptAt = -1);
        void stop();
        void onFrame(float dt);

        int getHours() const;
        bool isRunning() const;

        // signals
        typedef MyGUI::delegates::MultiDelegate<> EventHandle_Void;
        typedef MyGUI::delegates::MultiDelegate<int, int> EventHandle_IntInt;

        EventHandle_IntInt eventProgressChanged;
        EventHandle_Void eventInterrupted;
        EventHandle_Void eventFinished;

    private:
        bool mRunning;

        int mCurHour;
        int mHours;
        int mInterruptAt;

        float mRemainingTime;
    };
}

#endif
