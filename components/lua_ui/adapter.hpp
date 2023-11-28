#ifndef OPENMW_LUAUI_ADAPTER
#define OPENMW_LUAUI_ADAPTER

#include <memory>

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

    private:
        std::shared_ptr<Element> mElement;
        LuaContainer* mContainer;

        void attachElement();
        void detachElement();

        void containerChangedCoord(MyGUI::Widget*);
    };
}

#endif // !OPENMW_LUAUI_ADAPTER
