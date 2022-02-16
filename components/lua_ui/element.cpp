#include "element.hpp"

#include <MyGUI_Gui.h>

#include "content.hpp"
#include "util.hpp"
#include "widget.hpp"

namespace LuaUi
{
    namespace LayoutKeys
    {
        constexpr std::string_view type = "type";
        constexpr std::string_view name = "name";
        constexpr std::string_view layer = "layer";
        constexpr std::string_view templateLayout = "template";
        constexpr std::string_view props = "props";
        constexpr std::string_view events = "events";
        constexpr std::string_view content = "content";
        constexpr std::string_view external = "external";
    }

    std::string widgetType(const sol::table& layout)
    {
        return layout.get_or(LayoutKeys::type, std::string("LuaWidget"));
    }

    void destroyWidget(LuaUi::WidgetExtension* ext)
    {
        ext->deinitialize();
        MyGUI::Gui::getInstancePtr()->destroyWidget(ext->widget());
    }

    WidgetExtension* createWidget(const sol::table& layout);
    void updateWidget(WidgetExtension* ext, const sol::table& layout);

    std::vector<WidgetExtension*> updateContent(
        const std::vector<WidgetExtension*>& children, const sol::object& contentObj)
    {
        std::vector<WidgetExtension*> result;
        if (contentObj == sol::nil)
        {
            for (WidgetExtension* w : children)
                destroyWidget(w);
            return result;
        }
        if (!contentObj.is<Content>())
            throw std::logic_error("Layout content field must be a openmw.ui.content");
        Content content = contentObj.as<Content>();
        result.resize(content.size());
        size_t minSize = std::min(children.size(), content.size());
        for (size_t i = 0; i < minSize; i++)
        {
            WidgetExtension* ext = children[i];
            sol::table newLayout = content.at(i);
            if (ext->widget()->getTypeName() == widgetType(newLayout)
                && ext->getLayout() == newLayout)
            {
                updateWidget(ext, newLayout);
            }
            else
            {
                destroyWidget(ext);
                ext = createWidget(newLayout);
            }
            result[i] = ext;
        }
        for (size_t i = minSize; i < children.size(); i++)
            destroyWidget(children[i]);
        for (size_t i = minSize; i < content.size(); i++)
            result[i] = createWidget(content.at(i));
        return result;
    }

    void setTemplate(WidgetExtension* ext, const sol::object& templateLayout)
    {
        // \todo remove when none of the widgets require this workaround
        sol::object skin = LuaUtil::getFieldOrNil(templateLayout, "skin");
        if (skin.is<std::string>())
            ext->widget()->changeWidgetSkin(skin.as<std::string>());

        sol::object props = LuaUtil::getFieldOrNil(templateLayout, LayoutKeys::props);
        ext->setTemplateProperties(props);
        sol::object content = LuaUtil::getFieldOrNil(templateLayout, LayoutKeys::content);
        ext->setTemplateChildren(updateContent(ext->templateChildren(), content));
    }

    void setEventCallbacks(LuaUi::WidgetExtension* ext, const sol::object& eventsObj)
    {
        ext->clearCallbacks();
        if (eventsObj == sol::nil)
            return;
        if (!eventsObj.is<sol::table>())
            throw std::logic_error("The \"events\" layout field must be a table of callbacks");
        auto events = eventsObj.as<sol::table>();
        events.for_each([ext](const sol::object& name, const sol::object& callback)
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

    WidgetExtension* createWidget(const sol::table& layout)
    {
        std::string type = widgetType(layout);
        std::string name = layout.get_or(LayoutKeys::name, std::string());

        static auto widgetTypeMap = widgetTypeToName();
        if (widgetTypeMap.find(type) == widgetTypeMap.end())
            throw std::logic_error(std::string("Invalid widget type ") += type);

        MyGUI::Widget* widget = MyGUI::Gui::getInstancePtr()->createWidgetT(
            type, "",
            MyGUI::IntCoord(), MyGUI::Align::Default,
            std::string(), name);

        WidgetExtension* ext = dynamic_cast<WidgetExtension*>(widget);
        if (!ext)
            throw std::runtime_error("Invalid widget!");
        ext->initialize(layout.lua_state(), widget);

        updateWidget(ext, layout);
        return ext;
    }

    void updateWidget(WidgetExtension* ext, const sol::table& layout)
    {
        ext->resetSlot(); // otherwise if template gets changed, all non-template children will get destroyed

        ext->setLayout(layout);
        ext->setExternal(layout.get<sol::object>(LayoutKeys::external));
        setTemplate(ext, layout.get<sol::object>(LayoutKeys::templateLayout));
        ext->setProperties(layout.get<sol::object>(LayoutKeys::props));
        setEventCallbacks(ext, layout.get<sol::object>(LayoutKeys::events));

        ext->setChildren(updateContent(ext->children(), layout.get<sol::object>(LayoutKeys::content)));
    }

    std::string setLayer(WidgetExtension* ext, const sol::table& layout)
    {
        MyGUI::ILayer* layerNode = ext->widget()->getLayer();
        std::string currentLayer = layerNode ? layerNode->getName() : std::string();
        std::string newLayer = layout.get_or(LayoutKeys::layer, std::string());
        if (!newLayer.empty() && !MyGUI::LayerManager::getInstance().isExist(newLayer))
            throw std::logic_error(std::string("Layer ") + newLayer + " doesn't exist");
        else if (newLayer != currentLayer)
        {
            MyGUI::LayerManager::getInstance().attachToLayerNode(newLayer, ext->widget());
        }
        return newLayer;
    }

    std::map<Element*, std::shared_ptr<Element>> Element::sAllElements;

    Element::Element(sol::table layout)
        : mRoot(nullptr)
        , mAttachedTo(nullptr)
        , mLayout(std::move(layout))
        , mLayer()
        , mUpdate(false)
        , mDestroy(false)
    {}


    std::shared_ptr<Element> Element::make(sol::table layout)
    {
        std::shared_ptr<Element> ptr(new Element(std::move(layout)));
        sAllElements[ptr.get()] = ptr;
        return ptr;
    }

    void Element::create()
    {
        assert(!mRoot);
        if (!mRoot)
        {
            mRoot = createWidget(mLayout);
            mLayer = setLayer(mRoot, mLayout);
            updateAttachment();
        }
    }

    void Element::update()
    {
        if (mRoot && mUpdate)
        {
            if (mRoot->widget()->getTypeName() != widgetType(mLayout))
            {
                destroyWidget(mRoot);
                mRoot = createWidget(mLayout);
            }
            else
            {
                updateWidget(mRoot, mLayout);
            }
            mLayer = setLayer(mRoot, mLayout);
            updateAttachment();
        }
        mUpdate = false;
    }

    void Element::destroy()
    {
        if (mRoot)
            destroyWidget(mRoot);
        mRoot = nullptr;
        sAllElements.erase(this);
    }

    void Element::attachToWidget(WidgetExtension* w)
    {
        if (mAttachedTo)
            throw std::logic_error("A UI element can't be attached to two widgets at once");
        mAttachedTo = w;
        updateAttachment();
    }

    void Element::detachFromWidget()
    {
        if (mRoot)
            mRoot->widget()->detachFromWidget();
        if (mAttachedTo)
            mAttachedTo->setChildren({});
        mAttachedTo = nullptr;
    }

    void Element::updateAttachment()
    {
        if (!mRoot)
            return;
        if (mAttachedTo)
        {
            if (!mLayer.empty())
                Log(Debug::Warning) << "Ignoring element's layer " << mLayer << " because it's attached to a widget";
            mAttachedTo->setChildren({ mRoot });
            mRoot->updateCoord();
        }
    }
}
