#include "exposedwindow.hpp"

namespace MWGui
{
    MyGUI::VectorWidgetPtr Window::getSkinWidgetsByName(const std::string& name)
    {
        return MyGUI::Widget::getSkinWidgetsByName(name);
    }

    MyGUI::Widget* Window::getSkinWidget(const std::string& name)
    {
        MyGUI::VectorWidgetPtr widgets = getSkinWidgetsByName(name);

        if (widgets.empty())
        {
            MYGUI_ASSERT(false, "widget name '" << name << "' not found in skin of layout '" << getName() << "'");
            return nullptr;
        }
        else
        {
            return widgets[0];
        }
    }
}
