#ifndef OPENMW_LUAUI_ELEMENT
#define OPENMW_LUAUI_ELEMENT

#include "widget.hpp"

namespace LuaUi
{
    struct Element
    {
        static std::shared_ptr<Element> make(sol::table layout);

        WidgetExtension* mRoot;
        WidgetExtension* mAttachedTo;
        sol::table mLayout;
        std::string mLayer;
        bool mUpdate;
        bool mDestroy;

        void create();

        void update();

        void destroy();

        friend void clearUserInterface();

        void attachToWidget(WidgetExtension* w);
        void detachFromWidget();

        private:
            Element(sol::table layout);
            static std::map<Element*, std::shared_ptr<Element>> sAllElements;
            void updateAttachment();
    };
}

#endif // !OPENMW_LUAUI_ELEMENT
