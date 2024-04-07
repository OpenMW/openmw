#ifndef OPENMW_LUAUI_ELEMENT
#define OPENMW_LUAUI_ELEMENT

#include "widget.hpp"

namespace LuaUi
{
    struct Element
    {
        static std::shared_ptr<Element> make(sol::table layout, bool menu);
        static void erase(Element* element);

        template <class Callback>
        static void forEach(bool menu, Callback callback)
        {
            auto& container = menu ? sMenuElements : sGameElements;
            for (auto& [_, element] : container)
                callback(element.get());
        }

        WidgetExtension* mRoot;
        sol::object mLayout;
        std::string mLayer;

        enum State
        {
            New,
            Created,
            Update,
            Destroy,
            Destroyed,
        };
        State mState;

        void create(uint64_t dept = 0);

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
