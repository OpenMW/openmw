#include "exposedwindow.hpp"

namespace MWGui
{
    MyGUI::VectorWidgetPtr Window::getSkinWidgetsByName (const std::string &name)
    {
        return MyGUI::Widget::getSkinWidgetsByName (name);
    }

    MyGUI::Widget* Window::getSkinWidget(const std::string & _name, bool _throw)
    {
        MyGUI::VectorWidgetPtr widgets = getSkinWidgetsByName (_name);

        if (widgets.empty())
        {
            MYGUI_ASSERT( ! _throw, "widget name '" << _name << "' not found in skin of layout '" << getName() << "'");
            return nullptr;
        }
        else
        {
            return widgets[0];
        }
    }
}
