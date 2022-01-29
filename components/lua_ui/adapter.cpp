#include "adapter.hpp"

#include <MyGUI_Gui.h>

#include "element.hpp"
#include "container.hpp"

namespace LuaUi
{
    namespace
    {
        sol::state luaState;
    }

    LuaAdapter::LuaAdapter()
        : mElement(nullptr)
        , mContent(nullptr)
    {
        mContent = MyGUI::Gui::getInstancePtr()->createWidget<LuaContainer>(
            "", MyGUI::IntCoord(), MyGUI::Align::Default, "", "");
        mContent->initialize(luaState, mContent);
        mContent->onCoordChange([this](WidgetExtension* ext, MyGUI::IntCoord coord)
        {
            setSize(coord.size());
        });
        mContent->widget()->attachToWidget(this);
    }

    void LuaAdapter::attach(const std::shared_ptr<Element>& element)
    {
        detachElement();
        mElement = element;
        attachElement();
        setSize(mContent->widget()->getSize());

        // workaround for MyGUI bug
        // parent visibility doesn't affect added children
        setVisible(!getVisible());
        setVisible(!getVisible());
    }

    void LuaAdapter::detach()
    {
        detachElement();
        setSize({ 0, 0 });
    }

    void LuaAdapter::attachElement()
    {
        if (mElement.get())
            mElement->attachToWidget(mContent);
    }

    void LuaAdapter::detachElement()
    {
        if (mElement.get())
            mElement->detachFromWidget();
        mElement = nullptr;
    }
}
