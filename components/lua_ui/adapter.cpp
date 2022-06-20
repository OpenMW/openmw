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
        , mContainer(nullptr)
    {
        mContainer = MyGUI::Gui::getInstancePtr()->createWidget<LuaContainer>(
            "", MyGUI::IntCoord(), MyGUI::Align::Default, "", "");
        mContainer->initialize(luaState, mContainer);
        mContainer->onCoordChange([this](WidgetExtension* ext, MyGUI::IntCoord coord)
        {
            setSize(coord.size());
        });
        mContainer->widget()->attachToWidget(this);
    }

    void LuaAdapter::attach(const std::shared_ptr<Element>& element)
    {
        detachElement();
        mElement = element;
        attachElement();
        setSize(mContainer->widget()->getSize());

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
            mElement->attachToWidget(mContainer);
    }

    void LuaAdapter::detachElement()
    {
        if (mElement.get())
            mElement->detachFromWidget();
        mElement = nullptr;
    }
}

