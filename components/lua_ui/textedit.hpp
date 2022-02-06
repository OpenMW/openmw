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
            void initialize() override;
            void deinitialize() override;
            void updateProperties() override;

        private:
            void textChange(MyGUI::EditBox*);
    };
}

#endif // OPENMW_LUAUI_TEXTEDIT
