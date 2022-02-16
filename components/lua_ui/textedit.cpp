#include "textedit.hpp"

#include "alignment.hpp"

namespace LuaUi
{
    void LuaTextEdit::initialize()
    {
        changeWidgetSkin("LuaTextEdit");

        eventEditTextChange += MyGUI::newDelegate(this, &LuaTextEdit::textChange);

        WidgetExtension::initialize();
    }

    void LuaTextEdit::deinitialize()
    {
        eventEditTextChange -= MyGUI::newDelegate(this, &LuaTextEdit::textChange);
        WidgetExtension::deinitialize();
    }

    void LuaTextEdit::updateProperties()
    {
        setCaption(propertyValue("text", std::string()));
        setFontHeight(propertyValue("textSize", 10));
        setTextColour(propertyValue("textColor", MyGUI::Colour(0, 0, 0, 1)));
        setEditMultiLine(propertyValue("multiline", false));
        setEditWordWrap(propertyValue("wordWrap", false));

        Alignment horizontal(propertyValue("textAlignH", Alignment::Start));
        Alignment vertical(propertyValue("textAlignV", Alignment::Start));
        setTextAlign(alignmentToMyGui(horizontal, vertical));

        setEditStatic(propertyValue("readOnly", false));

        WidgetExtension::updateProperties();
    }

    void LuaTextEdit::textChange(MyGUI::EditBox*)
    {
        triggerEvent("textChanged", sol::make_object(lua(), getCaption().asUTF8()));
    }
}
