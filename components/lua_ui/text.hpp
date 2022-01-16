#ifndef OPENMW_LUAUI_TEXT
#define OPENMW_LUAUI_TEXT

#include <MyGUI_TextBox.h>

#include "widget.hpp"

namespace LuaUi
{
    class LuaText : public MyGUI::TextBox, public WidgetExtension
    {
        MYGUI_RTTI_DERIVED(LuaText)

        public:
            LuaText();
            virtual void initialize() override;
            virtual void setProperties(sol::object) override;

        private:
            bool mAutoSized;

        protected:
            virtual MyGUI::IntSize calculateSize() override;
    };
}

#endif // OPENMW_LUAUI_TEXT
