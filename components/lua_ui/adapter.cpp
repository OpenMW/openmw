#include "adapter.hpp"

#include <MyGUI_Gui.h>

#include "element.hpp"

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
        MyGUI::Widget* widget = MyGUI::Gui::getInstancePtr()->createWidgetT(
            "LuaWidget", "",
            MyGUI::IntCoord(), MyGUI::Align::Default,
            std::string(), "");

        mContent = dynamic_cast<WidgetExtension*>(widget);
        if (!mContent)
            throw std::runtime_error("Invalid widget!");
        mContent->initialize(luaState, widget);
        mContent->onSizeChange([this](MyGUI::IntSize size)
        {
            setSize(size);
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
