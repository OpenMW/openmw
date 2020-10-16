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
        MyGUI::TabControl* mTabControl;

        MyGUI::EditBox* mBulletProfilerEdit;
    };

}

#endif
