#ifndef OPENMW_MWGUI_DEBUGWINDOW_H
#define OPENMW_MWGUI_DEBUGWINDOW_H

#include "windowbase.hpp"

namespace MWGui
{

    class DebugWindow : public WindowBase
    {
    public:
        DebugWindow();

        void onFrame(float dt) override;

        static void startLogRecording();

    private:
        void updateLogView();
        void updateBulletProfile();

        MyGUI::TabControl* mTabControl;

        MyGUI::EditBox* mLogView;

        static std::vector<char> sLogCircularBuffer;
        static int64_t sLogStartIndex;
        static int64_t sLogEndIndex;

        MyGUI::EditBox* mBulletProfilerEdit;
    };

}

#endif
