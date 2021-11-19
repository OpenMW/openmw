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

        private:
            bool mAutoSized;

        protected:
            virtual MyGUI::IntSize calculateSize() override;
            bool setPropertyRaw(std::string_view name, sol::object value) override;
    };
}

#endif // OPENMW_LUAUI_TEXT
