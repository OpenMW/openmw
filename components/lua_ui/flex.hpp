#ifndef OPENMW_LUAUI_FLEX
#define OPENMW_LUAUI_FLEX

#include "widget.hpp"
#include "alignment.hpp"

namespace LuaUi
{
    class LuaFlex : public MyGUI::Widget, public WidgetExtension
    {
        MYGUI_RTTI_DERIVED(LuaFlex)

        protected:
            MyGUI::IntSize calculateSize() override;
            void updateProperties() override;
            void updateChildren() override;

        private:
            bool mHorizontal;
            bool mAutoSized;
            MyGUI::IntSize mChildrenSize;
            Alignment mAlign;
            Alignment mArrange;
    };
}

#endif // OPENMW_LUAUI_FLEX
