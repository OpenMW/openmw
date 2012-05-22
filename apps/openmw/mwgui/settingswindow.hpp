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

            void onOkButtonClicked(MyGUI::Widget* _sender);
    };
}

#endif

