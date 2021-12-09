
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

    void LuaText::setProperties(sol::object props)
    {
        setCaption(parseProperty(props, "caption", std::string()));
        mAutoSized = parseProperty(props, "autoSize", true);
        WidgetExtension::setProperties(props);
    }

    MyGUI::IntSize LuaText::calculateSize()
    {
        if (mAutoSized)
            return getTextSize();
        else
            return WidgetExtension::calculateSize();
    }
}
