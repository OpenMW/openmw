#ifndef MWGUI_CONTROLLERS_H
#define MWGUI_CONTROLLERS_H

#include <string>
#include <MyGUI_ControllerItem.h>

namespace MyGUI
{
    class Widget;
}

namespace MWGui
{
    namespace Controllers
    {
        /// Automatically positions a widget below the mouse cursor.
        class ControllerFollowMouse :
            public MyGUI::ControllerItem
        {
            MYGUI_RTTI_DERIVED( ControllerFollowMouse )

        private:
            bool addTime(MyGUI::Widget* _widget, float _time);
            void prepareItem(MyGUI::Widget* _widget);
        };
    }
}

#endif
