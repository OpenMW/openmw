#ifndef MWGUI_JOURNAL_H
#define MWGUI_JOURNAL_H

#include <sstream>
#include <set>
#include <string>
#include <utility>
#include "../mwdialogue/journal.hpp"

#include "window_base.hpp"

namespace MWGui
{
    class WindowManager;

    class JournalWindow : public WindowBase
    {
        public:
            JournalWindow(WindowManager& parWindowManager);

        private:
            enum ColorStyle
            {
                CS_Sub,
                CS_Normal,
                CS_Super
            };

            void onWindowResize(MyGUI::Window* window);

            void displayLeftText(std::string text);
            void displayRightText(std::string text);

            static const int lineHeight;

            MyGUI::WidgetPtr skillAreaWidget, skillClientWidget;
            MyGUI::VScrollPtr skillScrollerWidget;
            int lastPos, clientHeight;
            MyGUI::EditPtr mLeftTextWidget;
            MyGUI::EditPtr mRightTextWidget;
    };

}

#endif