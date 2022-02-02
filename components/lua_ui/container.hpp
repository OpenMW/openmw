#ifndef OPENMW_LUAUI_CONTAINER
#define OPENMW_LUAUI_CONTAINER

#include "widget.hpp"

namespace LuaUi
{
    class LuaContainer : public WidgetExtension, public MyGUI::Widget
    {
        MYGUI_RTTI_DERIVED(LuaContainer)

        protected:
            virtual void updateChildren() override;
            virtual MyGUI::IntSize childScalingSize() override;

        private:
            void updateSizeToFit();
    };
}

#endif // !OPENMW_LUAUI_CONTAINER
