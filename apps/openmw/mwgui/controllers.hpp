#ifndef MWGUI_CONTROLLERS_H
#define MWGUI_CONTROLLERS_H

#include <string>
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
            MYGUI_RTTI_DERIVED( ControllerFollowMouse )

        private:
            bool addTime(MyGUI::Widget* _widget, float _time) override;
            void prepareItem(MyGUI::Widget* _widget) override;
        };
    }

#endif
