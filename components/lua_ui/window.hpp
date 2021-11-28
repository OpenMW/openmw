#ifndef OPENMW_LUAUI_WINDOW
#define OPENMW_LUAUI_WINDOW

#include <optional>

#include <MyGUI_TextBox.h>

#include "widget.hpp"

namespace LuaUi
{
    class LuaWindow : public MyGUI::Widget, public WidgetExtension
    {
        MYGUI_RTTI_DERIVED(LuaWindow)

        public:
            LuaWindow();
            virtual void setProperties(sol::object) override;

        private:
            // \todo replace with LuaText when skins are properly implemented
            MyGUI::TextBox* mCaption;
            MyGUI::IntPoint mPreviousMouse;
            MyGUI::IntCoord mChangeScale;

            MyGUI::IntCoord mMoveResize;

        protected:
            virtual void initialize() override;
            virtual void deinitialize() override;

            void notifyMousePress(MyGUI::Widget*, int, int, MyGUI::MouseButton);
            void notifyMouseDrag(MyGUI::Widget*, int, int, MyGUI::MouseButton);
    };
}

#endif // OPENMW_LUAUI_WINDOW
