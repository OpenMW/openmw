#include "uibindings.hpp"

#include <components/lua/util.hpp>
#include <components/lua_ui/alignment.hpp>
#include <components/lua_ui/content.hpp>
#include <components/lua_ui/element.hpp>
#include <components/lua_ui/layers.hpp>
#include <components/lua_ui/registerscriptsettings.hpp>
#include <components/lua_ui/resources.hpp>
#include <components/lua_ui/util.hpp>

#include <components/misc/strings/format.hpp>
#include <components/settings/values.hpp>

#include "context.hpp"
#include "luamanagerimp.hpp"

#include "../mwbase/environment.hpp"
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

        const std::unordered_map<MWGui::GuiMode, std::string_view> modeToName{
            { MWGui::GM_Inventory, "Interface" },
            { MWGui::GM_Container, "Container" },
            { MWGui::GM_Companion, "Companion" },
            { MWGui::GM_MainMenu, "MainMenu" },
            { MWGui::GM_Journal, "Journal" },
            { MWGui::GM_Scroll, "Scroll" },
            { MWGui::GM_Book, "Book" },
            { MWGui::GM_Alchemy, "Alchemy" },
            { MWGui::GM_Repair, "Repair" },
            { MWGui::GM_Dialogue, "Dialogue" },
            { MWGui::GM_Barter, "Barter" },
            { MWGui::GM_Rest, "Rest" },
            { MWGui::GM_SpellBuying, "SpellBuying" },
            { MWGui::GM_Travel, "Travel" },
            { MWGui::GM_SpellCreation, "SpellCreation" },
            { MWGui::GM_Enchanting, "Enchanting" },
            { MWGui::GM_Recharge, "Recharge" },
            { MWGui::GM_Training, "Training" },
            { MWGui::GM_MerchantRepair, "MerchantRepair" },
            { MWGui::GM_Levelup, "LevelUp" },
            { MWGui::GM_Name, "ChargenName" },
            { MWGui::GM_Race, "ChargenRace" },
            { MWGui::GM_Birth, "ChargenBirth" },
            { MWGui::GM_Class, "ChargenClass" },
            { MWGui::GM_ClassGenerate, "ChargenClassGenerate" },
            { MWGui::GM_ClassPick, "ChargenClassPick" },
            { MWGui::GM_ClassCreate, "ChargenClassCreate" },
            { MWGui::GM_Review, "ChargenClassReview" },
            { MWGui::GM_Loading, "Loading" },
            { MWGui::GM_LoadingWallpaper, "LoadingWallpaper" },
            { MWGui::GM_Jail, "Jail" },
            { MWGui::GM_QuickKeysMenu, "QuickKeysMenu" },
        };

        const auto nameToMode = [] {
            std::unordered_map<std::string_view, MWGui::GuiMode> res;
            for (const auto& [mode, name] : modeToName)
                res[name] = mode;
            return res;
        }();
    }

    sol::table registerUiApi(const Context& context)
    {
        sol::state_view lua = context.sol();
        bool menu = context.mType == Context::Menu;

        MWBase::WindowManager* windowManager = MWBase::Environment::get().getWindowManager();

        sol::table api(lua, sol::create);
        api["_setHudVisibility"] = [luaManager = context.mLuaManager](bool state) {
            luaManager->addAction([state] { MWBase::Environment::get().getWindowManager()->setHudVisibility(state); });
        };
        api["_isHudVisible"] = []() -> bool { return MWBase::Environment::get().getWindowManager()->isHudVisible(); };
        api["showMessage"]
            = [luaManager = context.mLuaManager](std::string_view message, const sol::optional<sol::table>& options) {
                  MWGui::ShowInDialogueMode mode = MWGui::ShowInDialogueMode_IfPossible;
                  if (options.has_value())
                  {
                      auto showInDialogue = options->get<sol::optional<bool>>("showInDialogue");
                      if (showInDialogue.has_value())
                      {
                          if (*showInDialogue)
                              mode = MWGui::ShowInDialogueMode_Only;
                          else
                              mode = MWGui::ShowInDialogueMode_Never;
                      }
                  }
                  luaManager->addUIMessage(message, mode);
              };

        api["_showInteractiveMessage"] = [windowManager](std::string_view message, sol::optional<sol::table>) {
            windowManager->interactiveMessageBox(message, { "#{Interface:OK}" });
        };
        api["CONSOLE_COLOR"] = LuaUtil::makeStrictReadOnly(LuaUtil::tableFromPairs<std::string, Misc::Color>(lua,
            {
                { "Default", Misc::Color::fromHex(MWBase::WindowManager::sConsoleColor_Default.substr(1)) },
                { "Error", Misc::Color::fromHex(MWBase::WindowManager::sConsoleColor_Error.substr(1)) },
                { "Success", Misc::Color::fromHex(MWBase::WindowManager::sConsoleColor_Success.substr(1)) },
                { "Info", Misc::Color::fromHex(MWBase::WindowManager::sConsoleColor_Info.substr(1)) },
            }));
        api["printToConsole"]
            = [luaManager = context.mLuaManager](const std::string& message, const Misc::Color& color) {
                  luaManager->addInGameConsoleMessage(message + "\n", color);
              };
        api["setConsoleMode"] = [luaManager = context.mLuaManager, windowManager](std::string_view mode) {
            luaManager->addAction([mode = std::string(mode), windowManager] { windowManager->setConsoleMode(mode); });
        };
        api["getConsoleMode"] = [windowManager]() -> std::string_view { return windowManager->getConsoleMode(); };
        api["setConsoleSelectedObject"] = [luaManager = context.mLuaManager, windowManager](const sol::object& obj) {
            if (obj == sol::nil)
                luaManager->addAction([windowManager] { windowManager->setConsoleSelectedObject(MWWorld::Ptr()); });
            else
            {
                if (!obj.is<LObject>())
                    throw std::runtime_error("Game object expected");
                luaManager->addAction(
                    [windowManager, obj = obj.as<LObject>()] { windowManager->setConsoleSelectedObject(obj.ptr()); });
            }
        };
        api["content"] = LuaUi::loadContentConstructor(context.mLua);

        api["create"] = [luaManager = context.mLuaManager, menu](const sol::table& layout) {
            auto element = LuaUi::Element::make(layout, menu);
            luaManager->addAction([element] { wrapAction(element, [&] { element->create(); }); }, "Create UI");
            return element;
        };

        api["updateAll"] = [luaManager = context.mLuaManager, menu]() {
            LuaUi::Element::forEach(menu, [](LuaUi::Element* e) {
                if (e->mState == LuaUi::Element::Created)
                    e->mState = LuaUi::Element::Update;
            });
            luaManager->addAction([menu]() { LuaUi::Element::forEach(menu, [](LuaUi::Element* e) { e->update(); }); },
                "Update all menu UI elements");
        };
        api["_getMenuTransparency"] = []() -> float { return Settings::gui().mMenuTransparency; };

        sol::table layersTable(lua, sol::create);
        layersTable["indexOf"] = [](std::string_view name) -> sol::optional<size_t> {
            size_t index = LuaUi::Layer::indexOf(name);
            if (index == LuaUi::Layer::count())
                return sol::nullopt;
            else
                return LuaUtil::toLuaIndex(index);
        };
        layersTable["insertAfter"] = [context](std::string afterName, std::string name, const sol::object& opt) {
            LuaUi::Layer::Options options;
            options.mInteractive = LuaUtil::getValueOrDefault(LuaUtil::getFieldOrNil(opt, "interactive"), true);
            context.mLuaManager->addAction(
                [afterName = std::move(afterName), name = std::move(name), options]() {
                    size_t index = LuaUi::Layer::indexOf(afterName);
                    if (index == LuaUi::Layer::count())
                        throw std::logic_error(
                            Misc::StringUtils::format("Couldn't insert after non-existent layer %s", afterName));
                    LuaUi::Layer::insert(index + 1, name, options);
                },
                "Insert after UI layer");
        };
        layersTable["insertBefore"] = [context](std::string beforeName, std::string name, const sol::object& opt) {
            LuaUi::Layer::Options options;
            options.mInteractive = LuaUtil::getValueOrDefault(LuaUtil::getFieldOrNil(opt, "interactive"), true);
            context.mLuaManager->addAction(
                [beforeName = std::move(beforeName), name = std::move(name), options]() {
                    size_t index = LuaUi::Layer::indexOf(beforeName);
                    if (index == LuaUi::Layer::count())
                        throw std::logic_error(
                            Misc::StringUtils::format("Couldn't insert before non-existent layer %s", beforeName));
                    LuaUi::Layer::insert(index, name, options);
                },
                "Insert before UI layer");
        };
        sol::table layers = LuaUtil::makeReadOnly(layersTable);
        sol::table layersMeta = layers[sol::metatable_key];
        layersMeta[sol::meta_function::length] = []() { return LuaUi::Layer::count(); };
        layersMeta[sol::meta_function::index] = sol::overload(
            [](const sol::object& self, size_t index) {
                index = LuaUtil::fromLuaIndex(index);
                return LuaUi::Layer(index);
            },
            [layersTable](
                const sol::object& self, std::string_view key) { return layersTable.raw_get<sol::object>(key); });
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
            layersMeta[sol::meta_function::pairs] = pairs;
            layersMeta[sol::meta_function::ipairs] = pairs;
        }
        api["layers"] = layers;

        sol::table typeTable(lua, sol::create);
        for (const auto& it : LuaUi::widgetTypeToName())
            typeTable.set(it.second, it.first);
        api["TYPE"] = LuaUtil::makeStrictReadOnly(typeTable);

        api["ALIGNMENT"] = LuaUtil::makeStrictReadOnly(LuaUtil::tableFromPairs<std::string_view, LuaUi::Alignment>(lua,
            { { "Start", LuaUi::Alignment::Start }, { "Center", LuaUi::Alignment::Center },
                { "End", LuaUi::Alignment::End } }));

        api["registerSettingsPage"] = &LuaUi::registerSettingsPage;
        api["removeSettingsPage"] = &LuaUi::removeSettingsPage;

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
            return luaManager->uiResourceManager()->registerTexture(std::move(data));
        };

        api["screenSize"] = []() { return osg::Vec2f(Settings::video().mResolutionX, Settings::video().mResolutionY); };

        api["_getAllUiModes"] = [](sol::this_state lua) {
            sol::table res(lua, sol::create);
            for (const auto& [_, name] : modeToName)
                res[name] = name;
            return res;
        };
        api["_getUiModeStack"] = [windowManager](sol::this_state lua) {
            sol::table res(lua, sol::create);
            int i = 1;
            for (MWGui::GuiMode m : windowManager->getGuiModeStack())
                res[i++] = modeToName.at(m);
            return res;
        };
        api["_setUiModeStack"]
            = [windowManager, luaManager = context.mLuaManager](sol::table modes, sol::optional<LObject> arg) {
                  std::vector<MWGui::GuiMode> newStack(modes.size());
                  for (unsigned i = 0; i < newStack.size(); ++i)
                      newStack[i] = nameToMode.at(LuaUtil::cast<std::string_view>(modes[LuaUtil::toLuaIndex(i)]));
                  luaManager->addAction(
                      [windowManager, newStack = std::move(newStack), arg = std::move(arg)]() {
                          MWWorld::Ptr ptr;
                          if (arg.has_value())
                              ptr = arg->ptr();
                          const std::vector<MWGui::GuiMode>& stack = windowManager->getGuiModeStack();
                          unsigned common = 0;
                          while (common < std::min(stack.size(), newStack.size()) && stack[common] == newStack[common])
                              common++;
                          // TODO: Maybe disallow opening/closing special modes (main menu, settings, loading screen)
                          // from player scripts. Add new Lua context "menu" that can do it.
                          for (unsigned i = stack.size() - common; i > 0; i--)
                              windowManager->popGuiMode(true);
                          if (common == newStack.size() && !newStack.empty() && arg.has_value())
                              windowManager->pushGuiMode(newStack.back(), ptr);
                          for (unsigned i = common; i < newStack.size(); ++i)
                              windowManager->pushGuiMode(newStack[i], ptr);
                      },
                      "Set UI modes");
              };
        api["_getAllWindowIds"] = [windowManager](sol::this_state lua) {
            sol::table res(lua, sol::create);
            for (std::string_view name : windowManager->getAllWindowIds())
                res[name] = name;
            return res;
        };
        api["_getAllowedWindows"] = [windowManager](sol::this_state lua, std::string_view mode) {
            sol::table res(lua, sol::create);
            for (std::string_view name : windowManager->getAllowedWindowIds(nameToMode.at(mode)))
                res[name] = name;
            return res;
        };
        api["_setWindowDisabled"]
            = [windowManager, luaManager = context.mLuaManager](std::string window, bool disabled) {
                  luaManager->addAction(
                      [=, window = std::move(window)]() { windowManager->setDisabledByLua(window, disabled); });
              };
        api["_isWindowVisible"]
            = [windowManager](std::string_view window) { return windowManager->isWindowVisible(window); };

        // TODO
        // api["_showMouseCursor"] = [](bool) {};

        return api;
    }

    sol::table initUserInterfacePackage(const Context& context)
    {
        if (context.initializeOnce("openmw_ui_usertypes"))
        {
            auto element = context.sol().new_usertype<LuaUi::Element>("UiElement");
            element[sol::meta_function::to_string] = [](const LuaUi::Element& element) {
                std::stringstream res;
                res << "UiElement";
                if (element.mLayer != "")
                    res << "[" << element.mLayer << "]";
                return res.str();
            };
            element["layout"] = sol::property([](const LuaUi::Element& element) { return element.mLayout; },
                [](LuaUi::Element& element, const sol::main_table& layout) { element.mLayout = layout; });
            element["update"] = [luaManager = context.mLuaManager](const std::shared_ptr<LuaUi::Element>& element) {
                if (element->mState != LuaUi::Element::Created)
                    return;
                element->mState = LuaUi::Element::Update;
                luaManager->addAction([element] { wrapAction(element, [&] { element->update(); }); }, "Update UI");
            };
            element["destroy"] = [luaManager = context.mLuaManager](const std::shared_ptr<LuaUi::Element>& element) {
                if (element->mState == LuaUi::Element::Destroyed)
                    return;
                element->mState = LuaUi::Element::Destroy;
                luaManager->addAction(
                    [element] { wrapAction(element, [&] { LuaUi::Element::erase(element.get()); }); }, "Destroy UI");
            };

            auto uiLayer = context.sol().new_usertype<LuaUi::Layer>("UiLayer");
            uiLayer["name"]
                = sol::readonly_property([](LuaUi::Layer& self) -> std::string_view { return self.name(); });
            uiLayer["size"] = sol::readonly_property([](LuaUi::Layer& self) { return self.size(); });
            uiLayer[sol::meta_function::to_string]
                = [](LuaUi::Layer& self) { return Misc::StringUtils::format("UiLayer(%s)", self.name()); };
        }

        sol::object cached = context.getTypePackage("openmw_ui");
        if (cached != sol::nil)
            return cached;
        else
        {
            sol::table api = LuaUtil::makeReadOnly(registerUiApi(context));
            return context.setTypePackage(api, "openmw_ui");
        }
    }
}
