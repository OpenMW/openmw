#include "uibindings.hpp"

#include <components/lua_ui/alignment.hpp>
#include <components/lua_ui/content.hpp>
#include <components/lua_ui/element.hpp>
#include <components/lua_ui/layers.hpp>
#include <components/lua_ui/registerscriptsettings.hpp>
#include <components/lua_ui/resources.hpp>
#include <components/lua_ui/util.hpp>

#include <components/misc/strings/format.hpp>
#include <components/settings/settings.hpp>

#include "context.hpp"
#include "luamanagerimp.hpp"

#include "../mwbase/windowmanager.hpp"

namespace MWLua
{
    namespace
    {
        template <typename Fn>
        void wrapAction(const std::shared_ptr<LuaUi::Element>& element, Fn&& fn)
        {
            try
            {
                fn();
            }
            catch (...)
            {
                // prevent any actions on a potentially corrupted widget
                element->mRoot = nullptr;
                throw;
            }
        }

        // Lua arrays index from 1
        inline size_t fromLuaIndex(size_t i)
        {
            return i - 1;
        }
        inline size_t toLuaIndex(size_t i)
        {
            return i + 1;
        }
    }

    sol::table initUserInterfacePackage(const Context& context)
    {
        auto element = context.mLua->sol().new_usertype<LuaUi::Element>("Element");
        element["layout"] = sol::property([](LuaUi::Element& element) { return element.mLayout; },
            [](LuaUi::Element& element, const sol::table& layout) { element.mLayout = layout; });
        element["update"] = [luaManager = context.mLuaManager](const std::shared_ptr<LuaUi::Element>& element) {
            if (element->mDestroy || element->mUpdate)
                return;
            element->mUpdate = true;
            luaManager->addAction([element] { wrapAction(element, [&] { element->update(); }); }, "Update UI");
        };
        element["destroy"] = [luaManager = context.mLuaManager](const std::shared_ptr<LuaUi::Element>& element) {
            if (element->mDestroy)
                return;
            element->mDestroy = true;
            luaManager->addAction([element] { wrapAction(element, [&] { element->destroy(); }); }, "Destroy UI");
        };

        sol::table api = context.mLua->newTable();
        api["showMessage"]
            = [luaManager = context.mLuaManager](std::string_view message) { luaManager->addUIMessage(message); };
        api["CONSOLE_COLOR"] = LuaUtil::makeStrictReadOnly(context.mLua->tableFromPairs<std::string, Misc::Color>({
            { "Default", Misc::Color::fromHex(MWBase::WindowManager::sConsoleColor_Default.substr(1)) },
            { "Error", Misc::Color::fromHex(MWBase::WindowManager::sConsoleColor_Error.substr(1)) },
            { "Success", Misc::Color::fromHex(MWBase::WindowManager::sConsoleColor_Success.substr(1)) },
            { "Info", Misc::Color::fromHex(MWBase::WindowManager::sConsoleColor_Info.substr(1)) },
        }));
        api["printToConsole"]
            = [luaManager = context.mLuaManager](const std::string& message, const Misc::Color& color) {
                  luaManager->addInGameConsoleMessage(message + "\n", color);
              };
        api["setConsoleMode"] = [luaManager = context.mLuaManager](std::string_view mode) {
            luaManager->addAction(
                [mode = std::string(mode)] { MWBase::Environment::get().getWindowManager()->setConsoleMode(mode); });
        };
        api["setConsoleSelectedObject"] = [luaManager = context.mLuaManager](const sol::object& obj) {
            const auto wm = MWBase::Environment::get().getWindowManager();
            if (obj == sol::nil)
                luaManager->addAction([wm] { wm->setConsoleSelectedObject(MWWorld::Ptr()); });
            else
            {
                if (!obj.is<LObject>())
                    throw std::runtime_error("Game object expected");
                luaManager->addAction([wm, obj = obj.as<LObject>()] { wm->setConsoleSelectedObject(obj.ptr()); });
            }
        };
        api["content"] = LuaUi::loadContentConstructor(context.mLua);
        api["create"] = [luaManager = context.mLuaManager](const sol::table& layout) {
            auto element = LuaUi::Element::make(layout);
            luaManager->addAction([element] { wrapAction(element, [&] { element->create(); }); }, "Create UI");
            return element;
        };
        api["updateAll"] = [context]() {
            LuaUi::Element::forEach([](LuaUi::Element* e) { e->mUpdate = true; });
            context.mLuaManager->addAction(
                []() { LuaUi::Element::forEach([](LuaUi::Element* e) { e->update(); }); }, "Update all UI elements");
        };
        api["_getMenuTransparency"] = []() { return Settings::Manager::getFloat("menu transparency", "GUI"); };

        auto uiLayer = context.mLua->sol().new_usertype<LuaUi::Layer>("UiLayer");
        uiLayer["name"] = sol::property([](LuaUi::Layer& self) { return self.name(); });
        uiLayer["size"] = sol::property([](LuaUi::Layer& self) { return self.size(); });
        uiLayer[sol::meta_function::to_string]
            = [](LuaUi::Layer& self) { return Misc::StringUtils::format("UiLayer(%s)", self.name()); };

        sol::table layers = context.mLua->newTable();
        layers[sol::meta_function::length] = []() { return LuaUi::Layer::count(); };
        layers[sol::meta_function::index] = [](size_t index) {
            index = fromLuaIndex(index);
            return LuaUi::Layer(index);
        };
        layers["indexOf"] = [](std::string_view name) -> sol::optional<size_t> {
            size_t index = LuaUi::Layer::indexOf(name);
            if (index == LuaUi::Layer::count())
                return sol::nullopt;
            else
                return toLuaIndex(index);
        };
        layers["insertAfter"] = [context](std::string_view afterName, std::string_view name, const sol::object& opt) {
            LuaUi::Layer::Options options;
            options.mInteractive = LuaUtil::getValueOrDefault(LuaUtil::getFieldOrNil(opt, "interactive"), true);
            size_t index = LuaUi::Layer::indexOf(afterName);
            if (index == LuaUi::Layer::count())
                throw std::logic_error(std::string("Layer not found"));
            index++;
            context.mLuaManager->addAction([=]() { LuaUi::Layer::insert(index, name, options); }, "Insert UI layer");
        };
        layers["insertBefore"] = [context](std::string_view beforename, std::string_view name, const sol::object& opt) {
            LuaUi::Layer::Options options;
            options.mInteractive = LuaUtil::getValueOrDefault(LuaUtil::getFieldOrNil(opt, "interactive"), true);
            size_t index = LuaUi::Layer::indexOf(beforename);
            if (index == LuaUi::Layer::count())
                throw std::logic_error(std::string("Layer not found"));
            context.mLuaManager->addAction([=]() { LuaUi::Layer::insert(index, name, options); }, "Insert UI layer");
        };
        {
            auto pairs = [layers](const sol::object&) {
                auto next = [](const sol::table& l, size_t i) -> sol::optional<std::tuple<size_t, LuaUi::Layer>> {
                    if (i < LuaUi::Layer::count())
                        return std::make_tuple(i + 1, LuaUi::Layer(i));
                    else
                        return sol::nullopt;
                };
                return std::make_tuple(next, layers, 0);
            };
            layers[sol::meta_function::pairs] = pairs;
            layers[sol::meta_function::ipairs] = pairs;
        }
        api["layers"] = LuaUtil::makeReadOnly(layers);

        sol::table typeTable = context.mLua->newTable();
        for (const auto& it : LuaUi::widgetTypeToName())
            typeTable.set(it.second, it.first);
        api["TYPE"] = LuaUtil::makeStrictReadOnly(typeTable);

        api["ALIGNMENT"] = LuaUtil::makeStrictReadOnly(
            context.mLua->tableFromPairs<std::string_view, LuaUi::Alignment>({ { "Start", LuaUi::Alignment::Start },
                { "Center", LuaUi::Alignment::Center }, { "End", LuaUi::Alignment::End } }));

        api["registerSettingsPage"] = &LuaUi::registerSettingsPage;

        api["texture"] = [luaManager = context.mLuaManager](const sol::table& options) {
            LuaUi::TextureData data;
            sol::object path = LuaUtil::getFieldOrNil(options, "path");
            if (path.is<std::string>())
                data.mPath = path.as<std::string>();
            if (data.mPath.empty())
                throw std::logic_error("Invalid texture path");
            sol::object offset = LuaUtil::getFieldOrNil(options, "offset");
            if (offset.is<osg::Vec2f>())
                data.mOffset = offset.as<osg::Vec2f>();
            sol::object size = LuaUtil::getFieldOrNil(options, "size");
            if (size.is<osg::Vec2f>())
                data.mSize = size.as<osg::Vec2f>();
            return luaManager->uiResourceManager()->registerTexture(data);
        };

        api["screenSize"] = []() {
            return osg::Vec2f(
                Settings::Manager::getInt("resolution x", "Video"), Settings::Manager::getInt("resolution y", "Video"));
        };

        return LuaUtil::makeReadOnly(api);
    }
}
