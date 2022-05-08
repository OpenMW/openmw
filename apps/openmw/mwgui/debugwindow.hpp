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

    private:
        void initLogView();
        void updateLogView();
        void updateBulletProfile();

        MyGUI::TabControl* mTabControl;

        MyGUI::EditBox* mLogView;
        std::vector<char> mLogCircularBuffer;
        int64_t mLogStartIndex = 0;
        int64_t mLogEndIndex = 0;

        MyGUI::EditBox* mBulletProfilerEdit;
    };

}

#endif
