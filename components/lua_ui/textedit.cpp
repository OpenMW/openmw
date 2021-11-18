#include "textedit.hpp"

namespace LuaUi
{
    bool LuaTextEdit::setPropertyRaw(std::string_view name, sol::object value)
    {
        if (name == "caption")
        {
            if (!value.is<std::string>())
                return false;
            setCaption(value.as<std::string>());
        }
        else
        {
            return WidgetExtension::setPropertyRaw(name, value);
        }
        return true;
    }
}
