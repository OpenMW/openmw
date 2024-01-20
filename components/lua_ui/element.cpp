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
            sol::object templateTypeField
                = LuaUtil::getFieldOrNil(layout, LayoutKeys::templateLayout, LayoutKeys::type);
            if (templateTypeField != sol::nil)
            {
                std::string templateType = LuaUtil::getValueOrDefault(templateTypeField, defaultWidgetType);
                if (typeField != sol::nil && templateType != type)
                    throw std::logic_error(std::string("Template layout type ") + type
                        + std::string(" doesn't match template type ") + templateType);
                type = std::move(templateType);
            }
            return type;
        }

        void destroyWidget(WidgetExtension* ext)
        {
            ext->deinitialize();
            MyGUI::Gui::getInstancePtr()->destroyWidget(ext->widget());
        }

        void destroyChild(WidgetExtension* ext)
        {
            if (!ext->isRoot())
                destroyWidget(ext);
            else
                ext->widget()->detachFromWidget();
        }

        void detachElements(WidgetExtension* ext)
        {
            for (auto* child : ext->children())
            {
                if (child->isRoot())
                    child->widget()->detachFromWidget();
                else
                    detachElements(child);
            }
            for (auto* child : ext->templateChildren())
            {
                if (child->isRoot())
                    child->widget()->detachFromWidget();
                else
                    detachElements(child);
            }
        }

        void destroyRoot(WidgetExtension* ext)
        {
            detachElements(ext);
            destroyWidget(ext);
        }

        void updateRootCoord(WidgetExtension* ext)
        {
            WidgetExtension* root = ext;
            while (root->getParent())
                root = root->getParent();
            root->updateCoord();
        }

        WidgetExtension* pluckElementRoot(const sol::object& child)
        {
            std::shared_ptr<Element> element = child.as<std::shared_ptr<Element>>();
            WidgetExtension* root = element->mRoot;
            if (!root)
                throw std::logic_error("Using a destroyed element as a layout child");
            WidgetExtension* parent = root->getParent();
            if (parent)
            {
                auto children = parent->children();
                std::erase(children, root);
                parent->setChildren(children);
                root->widget()->detachFromWidget();
            }
            root->updateCoord();
            return root;
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
                    destroyChild(w);
                return result;
            }
            ContentView content(LuaUtil::cast<sol::table>(contentObj));
            result.resize(content.size());
            size_t minSize = std::min(children.size(), content.size());
            for (size_t i = 0; i < minSize; i++)
            {
                WidgetExtension* ext = children[i];
                sol::object child = content.at(i);
                if (child.is<Element>())
                {
                    WidgetExtension* root = pluckElementRoot(child);
                    if (ext != root)
                        destroyChild(ext);
                    result[i] = root;
                }
                else
                {
                    sol::table newLayout = child.as<sol::table>();
                    if (ext->widget()->getTypeName() == widgetType(newLayout))
                    {
                        updateWidget(ext, newLayout, depth);
                    }
                    else
                    {
                        destroyChild(ext);
                        ext = createWidget(newLayout, depth);
                    }
                    result[i] = ext;
                }
            }
            for (size_t i = minSize; i < children.size(); i++)
                destroyChild(children[i]);
            for (size_t i = minSize; i < content.size(); i++)
            {
                sol::object child = content.at(i);
                if (child.is<Element>())
                    result[i] = pluckElementRoot(child);
                else
                    result[i] = createWidget(child.as<sol::table>(), depth);
            }
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

        void setEventCallbacks(WidgetExtension* ext, const sol::object& eventsObj)
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
                type, "", MyGUI::IntCoord(), MyGUI::Align::Default, std::string(), name);

            WidgetExtension* ext = dynamic_cast<WidgetExtension*>(widget);
            if (!ext)
                throw std::runtime_error("Invalid widget!");
            ext->initialize(layout.lua_state(), widget, depth == 0);

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
        , mLayout(std::move(layout))
        , mLayer()
        , mUpdate(false)
        , mDestroy(false)
    {
    }

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
            updateRootCoord(mRoot);
        }
    }

    void Element::update()
    {
        if (mRoot && mUpdate)
        {
            if (mRoot->widget()->getTypeName() != widgetType(layout()))
            {
                destroyRoot(mRoot);
                WidgetExtension* parent = mRoot->getParent();
                auto children = parent->children();
                auto it = std::find(children.begin(), children.end(), mRoot);
                mRoot = createWidget(layout(), 0);
                assert(it != children.end());
                *it = mRoot;
                parent->setChildren(children);
                mRoot->updateCoord();
            }
            else
            {
                updateWidget(mRoot, layout(), 0);
            }
            mLayer = setLayer(mRoot, layout());
            updateRootCoord(mRoot);
        }
        mUpdate = false;
    }

    void Element::destroy()
    {
        if (mRoot)
        {
            destroyRoot(mRoot);
            mRoot = nullptr;
            mLayout = sol::make_object(mLayout.lua_state(), sol::nil);
        }
        sAllElements.erase(this);
    }
}
