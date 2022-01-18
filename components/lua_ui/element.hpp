#ifndef OPENMW_LUAUI_ELEMENT
#define OPENMW_LUAUI_ELEMENT

#include "widget.hpp"

namespace LuaUi
{
    struct Element
    {
        static std::shared_ptr<Element> make(sol::table layout);

        LuaUi::WidgetExtension* mRoot;
        sol::table mLayout;
        bool mUpdate;
        bool mDestroy;

        void create();

        void update();

        void destroy();

        friend void clearUserInterface();

        private:
            Element(sol::table layout);
            static std::map<Element*, std::shared_ptr<Element>> sAllElements;
    };
}

#endif // !OPENMW_LUAUI_ELEMENT
