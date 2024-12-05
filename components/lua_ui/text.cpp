#include "text.hpp"

#include "alignment.hpp"

namespace LuaUi
{
    LuaText::LuaText()
        : mAutoSized(true)
    {
    }

    void LuaText::initialize()
    {
        changeWidgetSkin("LuaText");
        setEditStatic(true);
        setVisibleHScroll(false);
        setVisibleVScroll(false);

        WidgetExtension::initialize();
    }

    void LuaText::updateProperties()
    {
        mAutoSized = propertyValue("autoSize", true);

        setCaption(propertyValue("text", std::string()));
        setFontHeight(propertyValue("textSize", 10));
        setTextColour(propertyValue("textColor", MyGUI::Colour(0, 0, 0, 1)));
        setEditMultiLine(propertyValue("multiline", false));
        setEditWordWrap(propertyValue("wordWrap", false));

        Alignment horizontal(propertyValue("textAlignH", Alignment::Start));
        Alignment vertical(propertyValue("textAlignV", Alignment::Start));
        setTextAlign(alignmentToMyGui(horizontal, vertical));

        setTextShadow(propertyValue("textShadow", false));
        setTextShadowColour(propertyValue("textShadowColor", MyGUI::Colour(0, 0, 0, 1)));

        WidgetExtension::updateProperties();
    }

    void LuaText::setCaption(const MyGUI::UString& caption)
    {
        MyGUI::TextBox::setCaption(caption);
        if (mAutoSized)
            updateCoord();
    }

    MyGUI::IntSize LuaText::calculateSize() const
    {
        if (mAutoSized)
            return getTextSize();
        else
            return WidgetExtension::calculateSize();
    }
}
