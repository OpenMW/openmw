#ifndef MWGUI_CONTROLLERS_H
#define MWGUI_CONTROLLERS_H

#include <MyGUI_ControllerItem.h>

namespace MyGUI
{
    class Widget;
}

namespace MWGui::Controllers
{
    /// Automatically positions a widget below the mouse cursor.
    class ControllerFollowMouse final : public MyGUI::ControllerItem
    {
        MYGUI_RTTI_DERIVED(ControllerFollowMouse)

    private:
        bool addTime(MyGUI::Widget* widget, float time) override;
        void prepareItem(MyGUI::Widget* widget) override;
    };
}

#endif
