#include "element.hpp"

#include <MyGUI_Gui.h>

#include "content.hpp"
#include "widgetlist.hpp"

namespace LuaUi
{

    std::string widgetType(const sol::table& layout)
    {
        return layout.get_or("type", std::string("LuaWidget"));
    }

    Content content(const sol::table& layout)
    {
        auto optional = layout.get<sol::optional<Content>>("content");
        if (optional.has_value())
            return optional.value();
        else
            return Content();
    }

    void setProperties(LuaUi::WidgetExtension* ext, const sol::table& layout)
    {
        auto props = layout.get<sol::optional<sol::table>>("props");
        if (props.has_value())
        {
            props.value().for_each([ext](const sol::object& key, const sol::object& value)
                {
                    if (key.is<std::string_view>())
                        ext->setProperty(key.as<std::string_view>(), value);
                    else
                        Log(Debug::Warning) << "UI property key must be a string";
                });
            ext->updateCoord();
        }
    }

    void setEventCallbacks(LuaUi::WidgetExtension* ext, const sol::table& layout)
    {
        ext->clearCallbacks();
        auto events = layout.get<sol::optional<sol::table>>("events");
        if (events.has_value())
        {
            events.value().for_each([ext](const sol::object& name, const sol::object& callback)
                {
                    if (name.is<std::string>() && callback.is<LuaUtil::Callback>())
                        ext->setCallback(name.as<std::string>(), callback.as<LuaUtil::Callback>());
                    else if (!name.is<std::string>())
                        Log(Debug::Warning) << "UI event key must be a string";
                    else if (!callback.is<LuaUtil::Callback>())
                        Log(Debug::Warning) << "UI event handler for key \"" << name.as<std::string>()
                                            << "\" must be an openmw.async.callback";
                });
        }
    }

    LuaUi::WidgetExtension* createWidget(const sol::table& layout, LuaUi::WidgetExtension* parent)
    {
        std::string type = widgetType(layout);
        std::string skin = layout.get_or("skin", std::string());
        std::string layer = layout.get_or("layer", std::string("Windows"));
        std::string name = layout.get_or("name", std::string());

        static auto widgetTypeMap = widgetTypeToName();
        if (widgetTypeMap.find(type) == widgetTypeMap.end())
            throw std::logic_error(std::string("Invalid widget type ") += type);

        MyGUI::Widget* widget = MyGUI::Gui::getInstancePtr()->createWidgetT(
            type, skin,
            MyGUI::IntCoord(), MyGUI::Align::Default,
            layer, name);

        LuaUi::WidgetExtension* ext = dynamic_cast<LuaUi::WidgetExtension*>(widget);
        if (!ext)
            throw std::runtime_error("Invalid widget!");

        ext->create(layout.lua_state(), widget);
        if (parent != nullptr)
            widget->attachToWidget(parent->widget());

        setEventCallbacks(ext, layout);
        setProperties(ext, layout);

        Content cont = content(layout);
        for (size_t i = 0; i < cont.size(); i++)
            ext->addChild(createWidget(cont.at(i), ext));

        return ext;
    }

    void destroyWidget(LuaUi::WidgetExtension* ext)
    {
        ext->destroy();
        MyGUI::Gui::getInstancePtr()->destroyWidget(ext->widget());
    }

    void updateWidget(const sol::table& layout, LuaUi::WidgetExtension* ext)
    {
        setEventCallbacks(ext, layout);
        setProperties(ext, layout);

        Content newContent = content(layout);

        size_t oldSize = ext->childCount();
        size_t newSize = newContent.size();
        size_t minSize = std::min(oldSize, newSize);
        for (size_t i = 0; i < minSize; i++)
        {
            LuaUi::WidgetExtension* oldWidget = ext->childAt(i);
            sol::table newChild = newContent.at(i);

            if (oldWidget->widget()->getTypeName() != widgetType(newChild))
            {
                destroyWidget(oldWidget);
                ext->assignChild(i, createWidget(newChild, ext));
            }
            else
                updateWidget(newChild, oldWidget);
        }

        for (size_t i = minSize; i < oldSize; i++)
            destroyWidget(ext->eraseChild(i));

        for (size_t i = minSize; i < newSize; i++)
            ext->addChild(createWidget(newContent.at(i), ext));
    }

    void Element::create()
    {
        assert(!mRoot);
        if (!mRoot)
            mRoot = createWidget(mLayout, nullptr);
    }

    void Element::update()
    {
        if (mRoot && mUpdate)
            updateWidget(mLayout, mRoot);
        mUpdate = false;
    }

    void Element::destroy()
    {
        if (mRoot)
            destroyWidget(mRoot);
        mRoot = nullptr;
    }
}
