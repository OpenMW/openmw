#include "textedit.hpp"

namespace LuaUi
{
    void LuaTextEdit::setProperties(sol::object props)
    {
        setCaption(parseProperty(props, "caption", std::string()));
        WidgetExtension::setProperties(props);
    }
}
