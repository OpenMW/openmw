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
            virtual void updateProperties() override;
            void setCaption(const MyGUI::UString& caption) override;

        private:
            bool mAutoSized;

        protected:
            virtual MyGUI::IntSize calculateSize() override;
    };
}

#endif // OPENMW_LUAUI_TEXT
