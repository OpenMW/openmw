#include "scrollbar.hpp"

#include <MyGUI_Button.h>

namespace Gui
{
    std::vector<MyGUI::Widget*> ScrollBar::getAllWidgets()
    {
        std::vector<MyGUI::Widget*> widgets;
        widgets.push_back(mWidgetStart);
        widgets.push_back(mWidgetEnd);
        widgets.push_back(mWidgetTrack);
        widgets.push_back(mWidgetFirstPart);
        widgets.push_back(mWidgetSecondPart);
        return widgets;
    }
}
