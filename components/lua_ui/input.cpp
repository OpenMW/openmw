#include "input.hpp"

#include "widget.hpp"

namespace LuaUi
{
    void dispatchMouseWheel(MyGUI::Widget* widget, float left, float top, float x, float y)
    {
        while (widget)
        {
            if (auto* ext = dynamic_cast<WidgetExtension*>(widget))
            {
                ext->mouseWheel(widget, left, top, x, y);
                break;
            }
            widget = widget->getParent();
        }
    }
}
