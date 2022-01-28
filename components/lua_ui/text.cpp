
#include "text.hpp"

namespace LuaUi
{
    LuaText::LuaText()
        : mAutoSized(true)
    {
        changeWidgetSkin("NormalText");
    }

    void LuaText::updateProperties()
    {
        setCaption(propertyValue("caption", std::string()));
        mAutoSized = propertyValue("autoSize", true);
        WidgetExtension::updateProperties();
    }

    void LuaText::setCaption(const MyGUI::UString& caption)
    {
        MyGUI::TextBox::setCaption(caption);
        if (mAutoSized)
            updateCoord();
    }

    MyGUI::IntSize LuaText::calculateSize()
    {
        if (mAutoSized)
            return getTextSize();
        else
            return WidgetExtension::calculateSize();
    }
}
