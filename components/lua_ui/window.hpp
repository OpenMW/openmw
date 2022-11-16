#ifndef OPENMW_LUAUI_WINDOW
#define OPENMW_LUAUI_WINDOW

#include <map>
#include <string>

#include <MyGUI_MouseButton.h>
#include <MyGUI_RTTI.h>
#include <MyGUI_Types.h>
#include <MyGUI_Widget.h>

#include "widget.hpp"

namespace LuaUi
{
    class LuaText;

    class LuaWindow : public MyGUI::Widget, public WidgetExtension
    {
        MYGUI_RTTI_DERIVED(LuaWindow)

    public:
        LuaWindow();
        void updateTemplate() override;
        void updateProperties() override;

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
