#ifndef OPENMW_LUAUI_ELEMENT
#define OPENMW_LUAUI_ELEMENT

#include "widget.hpp"

namespace LuaUi
{
    struct Element
    {
        static std::shared_ptr<Element> makeGameElement(sol::table layout);
        static std::shared_ptr<Element> makeMenuElement(sol::table layout);

        template <class Callback>
        static void forEachGameElement(Callback callback)
        {
            for (auto& [e, _] : sGameElements)
                callback(e);
        }

        template <class Callback>
        static void forEachMenuElement(Callback callback)
        {
            for (auto& [e, _] : sMenuElements)
                callback(e);
        }

        WidgetExtension* mRoot;
        sol::object mLayout;
        std::string mLayer;
        bool mUpdate;
        bool mDestroy;

        void create();

        void update();

        void destroy();

        friend void clearGameInterface();
        friend void clearMenuInterface();

    private:
        Element(sol::table layout);
        sol::table layout() { return LuaUtil::cast<sol::table>(mLayout); }
        static std::map<Element*, std::shared_ptr<Element>> sGameElements;
        static std::map<Element*, std::shared_ptr<Element>> sMenuElements;
    };
}

#endif // !OPENMW_LUAUI_ELEMENT
