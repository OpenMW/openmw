#ifndef OPENMW_LUAUI_TEXT
#define OPENMW_LUAUI_TEXT

#include <string>

#include <MyGUI_EditBox.h>
#include <MyGUI_RTTI.h>
#include <MyGUI_Types.h>
#include <MyGUI_UString.h>

#include "widget.hpp"

namespace LuaUi
{
    class LuaText : public MyGUI::EditBox, public WidgetExtension
    {
        MYGUI_RTTI_DERIVED(LuaText)

    public:
        LuaText();
        void initialize() override;
        void updateProperties() override;
        void setCaption(const MyGUI::UString& caption) override;

    private:
        bool mAutoSized;

    protected:
        MyGUI::IntSize calculateSize() override;
    };
}

#endif // OPENMW_LUAUI_TEXT
