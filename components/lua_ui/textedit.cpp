#include "textedit.hpp"

#include "alignment.hpp"

namespace LuaUi
{
    void LuaTextEdit::initialize()
    {
        mEditBox = createWidget<MyGUI::EditBox>("LuaTextEdit", MyGUI::IntCoord(0, 0, 0, 0), MyGUI::Align::Default);
        mEditBox->eventEditTextChange += MyGUI::newDelegate(this, &LuaTextEdit::textChange);
        registerEvents(mEditBox);
        WidgetExtension::initialize();
    }

    void LuaTextEdit::deinitialize()
    {
        mEditBox->eventEditTextChange -= MyGUI::newDelegate(this, &LuaTextEdit::textChange);
        clearEvents(mEditBox);
        WidgetExtension::deinitialize();
    }

    void LuaTextEdit::updateProperties()
    {
        mEditBox->setCaption(propertyValue("text", std::string()));
        mEditBox->setFontHeight(propertyValue("textSize", 10));
        mEditBox->setTextColour(propertyValue("textColor", MyGUI::Colour(0, 0, 0, 1)));
        mEditBox->setEditMultiLine(propertyValue("multiline", false));
        mEditBox->setEditWordWrap(propertyValue("wordWrap", false));

        Alignment horizontal(propertyValue("textAlignH", Alignment::Start));
        Alignment vertical(propertyValue("textAlignV", Alignment::Start));
        mEditBox->setTextAlign(alignmentToMyGui(horizontal, vertical));

        mEditBox->setEditStatic(propertyValue("readOnly", false));

        WidgetExtension::updateProperties();
    }

    void LuaTextEdit::textChange(MyGUI::EditBox*)
    {
        triggerEvent("textChanged", sol::make_object(lua(), mEditBox->getCaption().asUTF8()));
    }

    void LuaTextEdit::updateCoord()
    {
        WidgetExtension::updateCoord();
        {
            MyGUI::IntSize slotSize = slot()->calculateSize();
            MyGUI::IntPoint slotPosition = slot()->widget()->getAbsolutePosition() - widget()->getAbsolutePosition();
            MyGUI::IntCoord slotCoord(slotPosition, slotSize);
            mEditBox->setCoord(slotCoord);
        }
    }

    void LuaTextEdit::updateChildren()
    {
        WidgetExtension::updateChildren();
        // otherwise it won't be focusable
        mEditBox->detachFromWidget();
        mEditBox->attachToWidget(this);
    }
}
