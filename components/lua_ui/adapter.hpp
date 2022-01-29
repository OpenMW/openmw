#ifndef OPENMW_LUAUI_ADAPTER
#define OPENMW_LUAUI_ADAPTER

#include <MyGUI_Widget.h>

namespace LuaUi
{
    class LuaContainer;
    struct Element;
    class LuaAdapter : public MyGUI::Widget
    {
        MYGUI_RTTI_DERIVED(LuaAdapter)

        public:
            LuaAdapter();

            void attach(const std::shared_ptr<Element>& element);
            void detach();
            bool empty() { return mElement.get() == nullptr; }

        private:
            LuaContainer* mContent;
            std::shared_ptr<Element> mElement;
            void attachElement();
            void detachElement();
    };
}

#endif // !OPENMW_LUAUI_ADAPTER
