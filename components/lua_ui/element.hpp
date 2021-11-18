#ifndef OPENMW_LUAUI_ELEMENT
#define OPENMW_LUAUI_ELEMENT

#include "widget.hpp"

namespace LuaUi
{
    struct Element
    {
        Element(sol::table layout)
            : mRoot{ nullptr }
            , mLayout{ layout }
            , mUpdate{ false }
            , mDestroy{ false }
        {
        }

        LuaUi::WidgetExtension* mRoot;
        sol::table mLayout;
        bool mUpdate;
        bool mDestroy;

        void create();

        void update();

        void destroy();
    };
}

#endif // !OPENMW_LUAUI_ELEMENT
