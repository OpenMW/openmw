#include "element.hpp"

#include <MyGUI_Gui.h>

#include "content.hpp"
#include "util.hpp"
#include "widget.hpp"

namespace LuaUi
{
    namespace
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

        const std::string defaultWidgetType = "LuaWidget";

        constexpr uint64_t maxDepth = 250;

        std::string widgetType(const sol::table& layout)
        {
            sol::object typeField = LuaUtil::getFieldOrNil(layout, LayoutKeys::type);
            std::string type = LuaUtil::getValueOrDefault(typeField, defaultWidgetType);
            sol::object templateTypeField = LuaUtil::getFieldOrNil(layout, LayoutKeys::templateLayout, LayoutKeys::type);
            if (templateTypeField != sol::nil)
            {
                std::string templateType = LuaUtil::getValueOrDefault(templateTypeField, defaultWidgetType);
                if (typeField != sol::nil && templateType != type)
                    throw std::logic_error(std::string("Template layout type ") + type
                        + std::string(" doesn't match template type ") + templateType);
                type = templateType;
            }
            return type;
        }

        void destroyWidget(LuaUi::WidgetExtension* ext)
        {
            ext->deinitialize();
            MyGUI::Gui::getInstancePtr()->destroyWidget(ext->widget());
        }

        WidgetExtension* createWidget(const sol::table& layout, uint64_t depth);
        void updateWidget(WidgetExtension* ext, const sol::table& layout, uint64_t depth);

        std::vector<WidgetExtension*> updateContent(
            const std::vector<WidgetExtension*>& children, const sol::object& contentObj, uint64_t depth)
        {
            ++depth;
            std::vector<WidgetExtension*> result;
            if (contentObj == sol::nil)
            {
                for (WidgetExtension* w : children)
                    destroyWidget(w);
                return result;
            }
            ContentView content(LuaUtil::cast<sol::table>(contentObj));
            result.resize(content.size());
            size_t minSize = std::min(children.size(), content.size());
            for (size_t i = 0; i < minSize; i++)
            {
                WidgetExtension* ext = children[i];
                sol::table newLayout = content.at(i);
                if (ext->widget()->getTypeName() == widgetType(newLayout))
                {
                    updateWidget(ext, newLayout, depth);
                }
                else
                {
                    destroyWidget(ext);
                    ext = createWidget(newLayout, depth);
                }
                result[i] = ext;
            }
            for (size_t i = minSize; i < children.size(); i++)
                destroyWidget(children[i]);
            for (size_t i = minSize; i < content.size(); i++)
                result[i] = createWidget(content.at(i), depth);
            return result;
        }

        void setTemplate(WidgetExtension* ext, const sol::object& templateLayout, uint64_t depth)
        {
            ++depth;
            sol::object props = LuaUtil::getFieldOrNil(templateLayout, LayoutKeys::props);
            ext->setTemplateProperties(props);
            sol::object content = LuaUtil::getFieldOrNil(templateLayout, LayoutKeys::content);
            ext->setTemplateChildren(updateContent(ext->templateChildren(), content, depth));
        }

        void setEventCallbacks(LuaUi::WidgetExtension* ext, const sol::object& eventsObj)
        {
            ext->clearCallbacks();
            if (eventsObj == sol::nil)
                return;
            if (!eventsObj.is<sol::table>())
                throw std::logic_error("The \"events\" layout field must be a table of callbacks");
            auto events = eventsObj.as<sol::table>();
            events.for_each([ext](const sol::object& name, const sol::object& callback) {
                if (name.is<std::string>() && LuaUtil::Callback::isLuaCallback(callback))
                    ext->setCallback(name.as<std::string>(), LuaUtil::Callback::fromLua(callback));
                else if (!name.is<std::string>())
                    Log(Debug::Warning) << "UI event key must be a string";
                else
                    Log(Debug::Warning) << "UI event handler for key \"" << name.as<std::string>()
                                        << "\" must be an openmw.async.callback";
            });
        }

        WidgetExtension* createWidget(const sol::table& layout, uint64_t depth)
        {
            static auto widgetTypeMap = widgetTypeToName();
            std::string type = widgetType(layout);
            if (widgetTypeMap.find(type) == widgetTypeMap.end())
                throw std::logic_error(std::string("Invalid widget type ") += type);

            std::string name = layout.get_or(LayoutKeys::name, std::string());
            MyGUI::Widget* widget = MyGUI::Gui::getInstancePtr()->createWidgetT(
                type, "",
                MyGUI::IntCoord(), MyGUI::Align::Default,
                std::string(), name);

            WidgetExtension* ext = dynamic_cast<WidgetExtension*>(widget);
            if (!ext)
                throw std::runtime_error("Invalid widget!");
            ext->initialize(layout.lua_state(), widget);

            updateWidget(ext, layout, depth);
            return ext;
        }

        void updateWidget(WidgetExtension* ext, const sol::table& layout, uint64_t depth)
        {
            if (depth >= maxDepth)
                throw std::runtime_error("Maximum layout depth exceeded, probably caused by a circular reference");
            ext->reset();
            ext->setLayout(layout);
            ext->setExternal(layout.get<sol::object>(LayoutKeys::external));
            setTemplate(ext, layout.get<sol::object>(LayoutKeys::templateLayout), depth);
            ext->setProperties(layout.get<sol::object>(LayoutKeys::props));
            setEventCallbacks(ext, layout.get<sol::object>(LayoutKeys::events));
            ext->setChildren(updateContent(ext->children(), layout.get<sol::object>(LayoutKeys::content), depth));
            ext->updateCoord();
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
            mRoot = createWidget(layout(), 0);
            mLayer = setLayer(mRoot, layout());
            updateAttachment();
        }
    }

    void Element::update()
    {
        if (mRoot && mUpdate)
        {
            if (mRoot->widget()->getTypeName() != widgetType(layout()))
            {
                destroyWidget(mRoot);
                mRoot = createWidget(layout(), 0);
            }
            else
            {
                updateWidget(mRoot, layout(), 0);
            }
            mLayer = setLayer(mRoot, layout());
            updateAttachment();
        }
        mUpdate = false;
    }

    void Element::destroy()
    {
        sAllElements.erase(this);
        if (!mRoot)
            return;
        destroyWidget(mRoot);
        mRoot = nullptr;
        mLayout = sol::make_object(mLayout.lua_state(), sol::nil);
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
            mAttachedTo->updateCoord();
        }
    }
}
