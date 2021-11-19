#ifndef OPENMW_LUAUI_TEXTEDIT
#define OPENMW_LUAUI_TEXTEDIT

#include <MyGUI_EditBox.h>

#include "widget.hpp"

namespace LuaUi
{
    class LuaTextEdit : public MyGUI::EditBox, public WidgetExtension
    {
        MYGUI_RTTI_DERIVED(LuaTextEdit)

        protected:
            bool setPropertyRaw(std::string_view name, sol::object value) override;
    };
}

#endif // OPENMW_LUAUI_TEXTEDIT
