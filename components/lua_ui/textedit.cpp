#include "textedit.hpp"

#include "alignment.hpp"

namespace LuaUi
{
    void LuaTextEdit::initialize()
    {
        mEditBox = createWidget<MyGUI::EditBox>("LuaTextEdit", MyGUI::IntCoord(0, 0, 0, 0), MyGUI::Align::Default);
        mEditBox->eventEditTextChange += MyGUI::newDelegate(this, &LuaTextEdit::textChange);
        mEditBox->setMaxTextLength(std::numeric_limits<std::size_t>::max());
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
        mEditBox->setFontHeight(propertyValue("textSize", 10));
        mEditBox->setTextColour(propertyValue("textColor", MyGUI::Colour(0, 0, 0, 1)));
        mEditBox->setEditWordWrap(propertyValue("wordWrap", false));

        Alignment horizontal(propertyValue("textAlignH", Alignment::Start));
        Alignment vertical(propertyValue("textAlignV", Alignment::Start));
        mEditBox->setTextAlign(alignmentToMyGui(horizontal, vertical));

        mMultiline = propertyValue("multiline", false);
        mEditBox->setEditMultiLine(mMultiline);

        bool readOnly = propertyValue("readOnly", false);
        mEditBox->setEditStatic(readOnly);

        mAutoSize = (readOnly || !mMultiline) && propertyValue("autoSize", false);

        // change caption last, for multiline and wordwrap to apply
        mEditBox->setCaption(propertyValue("text", std::string()));

        WidgetExtension::updateProperties();
    }

    void LuaTextEdit::textChange(MyGUI::EditBox*)
    {
        protectedCall([=](LuaUtil::LuaView& view) {
            triggerEvent("textChanged", sol::make_object(view.sol(), mEditBox->getCaption().asUTF8()));
        });
    }

    void LuaTextEdit::updateCoord()
    {
        WidgetExtension::updateCoord();
        mEditBox->setSize(widget()->getSize());
    }

    void LuaTextEdit::updateChildren()
    {
        WidgetExtension::updateChildren();
        // otherwise it won't be focusable
        mEditBox->detachFromWidget();
        mEditBox->attachToWidget(this);
    }

    MyGUI::IntSize LuaTextEdit::calculateSize() const
    {
        MyGUI::IntSize normalSize = WidgetExtension::calculateSize();
        if (mAutoSize)
        {
            mEditBox->setSize(normalSize);
            int targetHeight = mMultiline ? mEditBox->getTextSize().height : mEditBox->getFontHeight();
            normalSize.height = std::max(normalSize.height, targetHeight);
        }
        return normalSize;
    }
}
