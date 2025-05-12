#ifndef OPENMW_COMPONENTS_WIDGETS_SCROLLBAR_H
#define OPENMW_COMPONENTS_WIDGETS_SCROLLBAR_H

#include <MyGUI_ScrollBar.h>

namespace Gui
{
    /// @brief A scrollbar that can return all its widgets for binding hover listeners.
    class ScrollBar : public MyGUI::ScrollBar
    {
        MYGUI_RTTI_DERIVED(ScrollBar)

    public:
        std::vector<MyGUI::Widget*> getAllWidgets();
    };
}

#endif
