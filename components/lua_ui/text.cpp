
#include "text.hpp"

namespace LuaUi
{
    LuaText::LuaText()
        : mAutoSized(true)
    {}

    void LuaText::initialize()
    {
        WidgetExtension::initialize();
    }

    bool LuaText::setPropertyRaw(std::string_view name, sol::object value)
    {
        if (name == "caption")
        {
            if (!value.is<std::string>())
                return false;
            setCaption(value.as<std::string>());
        }
        else if (name == "autoSize")
        {
            if (!value.is<bool>())
                return false;
            mAutoSized = value.as<bool>();
        }
        else
        {
            return WidgetExtension::setPropertyRaw(name, value);
        }
        return true;
    }

    MyGUI::IntSize LuaText::calculateSize()
    {
        if (mAutoSized)
            return getTextSize();
        else
            return WidgetExtension::calculateSize();
    }
}
