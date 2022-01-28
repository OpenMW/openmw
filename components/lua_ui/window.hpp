#ifndef OPENMW_LUAUI_WINDOW
#define OPENMW_LUAUI_WINDOW

#include <optional>

#include "widget.hpp"
#include "text.hpp"

namespace LuaUi
{
    class LuaWindow : public MyGUI::Widget, public WidgetExtension
    {
        MYGUI_RTTI_DERIVED(LuaWindow)

        public:
            LuaWindow();
            virtual void updateTemplate() override;
            virtual void updateProperties() override;

        private:
            LuaText* mCaption;
            std::map<MyGUI::Widget*, WidgetExtension*> mActionWidgets;
            MyGUI::IntPoint mPreviousMouse;
            MyGUI::IntCoord mChangeScale;

            MyGUI::IntCoord mMoveResize;

        protected:
            void notifyMousePress(MyGUI::Widget*, int, int, MyGUI::MouseButton);
            void notifyMouseDrag(MyGUI::Widget*, int, int, MyGUI::MouseButton);
    };
}

#endif // OPENMW_LUAUI_WINDOW
