#ifndef OPENMW_COMPONENTS_LUA_UI_INPUT_H
#define OPENMW_COMPONENTS_LUA_UI_INPUT_H

namespace MyGUI
{
    class Widget;
}

namespace LuaUi
{
    void dispatchMouseWheel(MyGUI::Widget* widget, float left, float top, float x, float y);
}

#endif
