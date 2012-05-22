#ifndef MWGUI_SETTINGS_H
#define MWGUI_SETTINGS_H

#include "window_base.hpp"

namespace MWGui
{
    class WindowManager;
}

namespace MWGui
{
    class SettingsWindow : public WindowBase
    {
        public:
            SettingsWindow(WindowManager& parWindowManager);

        protected:
            MyGUI::Button* mOkButton;
            MyGUI::ListBox* mResolutionList;
            MyGUI::ScrollBar* mMenuTransparencySlider;
            MyGUI::ScrollBar* mViewDistanceSlider;

            void onOkButtonClicked(MyGUI::Widget* _sender);
            void onSliderChangePosition(MyGUI::ScrollBar* scroller, size_t pos);
    };
}

#endif

