#ifndef MWGUI_TIMEADVANCER_H
#define MWGUI_TIMEADVANCER_H

#include <MyGUI_Widget.h>

namespace MWGui
{
    class TimeAdvancer
    {
        public:
            TimeAdvancer(float delay);

            void run(int hours, int interruptAt=-1);
            void stop();
            void onFrame(float dt);

            int getHours() const;
            bool isRunning() const;

            // signals
            typedef MyGUI::delegates::CMultiDelegate0 EventHandle_Void;
            typedef MyGUI::delegates::CMultiDelegate2<int, int> EventHandle_IntInt;

            EventHandle_IntInt eventProgressChanged;
            EventHandle_Void eventInterrupted;
            EventHandle_Void eventFinished;

        private:
            bool mRunning;

            int mCurHour;
            int mHours;
            int mInterruptAt;

            float mDelay;
            float mRemainingTime;
    };
}

#endif
