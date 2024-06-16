#include "mwscriptbindings.hpp"

#include <components/compiler/locals.hpp>
#include <components/lua/luastate.hpp>
#include <components/misc/strings/lower.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/scriptmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwscript/globalscripts.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/worldimp.hpp"

#include "object.hpp"

#include <stdexcept>

namespace MWLua
{
    struct MWScriptRef
    {
        ESM::RefId mId;
        sol::optional<GObject> mObj;

        MWScript::Locals& getLocals()
        {
            if (mObj)
                return mObj->ptr().getRefData().getLocals();
            else
                return MWBase::Environment::get().getScriptManager()->getGlobalScripts().getLocals(mId);
        }
        bool isRunning() const
        {
            if (mObj.has_value()) // local script
            {
                MWWorld::LocalScripts& localScripts = MWBase::Environment::get().getWorld()->getLocalScripts();
                return localScripts.isRunning(mId, mObj->ptr());
            }

            return MWBase::Environment::get().getScriptManager()->getGlobalScripts().isRunning(mId);
        }
    };
    struct MWScriptVariables
    {
        MWScriptRef mRef;
    };
}

namespace sol
{
    template <>
    struct is_automagical<MWLua::MWScriptRef> : std::false_type
    {
    };
    template <>
    struct is_automagical<MWLua::MWScriptVariables> : std::false_type
    {
    };
    template <>
    struct is_automagical<ESM::Global> : std::false_type
    {
    };
}

namespace MWLua
{

    float getGlobalVariableValue(const std::string_view globalId)
    {
        char varType = MWBase::Environment::get().getWorld()->getGlobalVariableType(globalId);
        if (varType == 'f')
        {
            return MWBase::Environment::get().getWorld()->getGlobalFloat(globalId);
        }
        else if (varType == 's' || varType == 'l')
        {
            return static_cast<float>(MWBase::Environment::get().getWorld()->getGlobalInt(globalId));
        }
        return 0;
    }

    void setGlobalVariableValue(const std::string_view globalId, float value)
    {
        char varType = MWBase::Environment::get().getWorld()->getGlobalVariableType(globalId);
        if (varType == 'f')
        {
            MWBase::Environment::get().getWorld()->setGlobalFloat(globalId, value);
        }
        else if (varType == 's' || varType == 'l')
        {
            MWBase::Environment::get().getWorld()->setGlobalInt(globalId, value);
        }
    }

    sol::table initMWScriptBindings(const Context& context)
    {
        sol::table api(context.mLua->sol(), sol::create);

        api["getGlobalScript"]
            = [](std::string_view recordId, sol::optional<GObject> player) -> sol::optional<MWScriptRef> {
            if (player.has_value() && player->ptr() != MWBase::Environment::get().getWorld()->getPlayerPtr())
                throw std::runtime_error("Second argument must either be a player or be missing");
            auto scriptId = ESM::RefId::deserializeText(recordId);
            if (MWBase::Environment::get().getScriptManager()->getGlobalScripts().getScriptIfPresent(scriptId))
                return MWScriptRef{ scriptId, sol::nullopt };
            else
                return sol::nullopt;
        };
        api["getLocalScript"] = [](const GObject& obj, sol::optional<GObject> player) -> sol::optional<MWScriptRef> {
            if (player.has_value() && player->ptr() != MWBase::Environment::get().getWorld()->getPlayerPtr())
                throw std::runtime_error("Second argument must either be a player or be missing");
            auto scriptId = obj.ptr().getRefData().getLocals().getScriptId();
            if (scriptId.empty())
                return sol::nullopt;
            return MWScriptRef{ scriptId, obj };
        };

        // In multiplayer it will be possible to have several instances (per player) of a single script,
        // so we will likely add functions returning Lua table of scripts.
        // api["getGlobalScripts"] = [](std::string_view recordId) -> list of scripts
        // api["getLocalScripts"] = [](const GObject& obj) -> list of scripts

        sol::state_view& lua = context.mLua->sol();
        sol::usertype<MWScriptRef> mwscript = context.mLua->sol().new_usertype<MWScriptRef>("MWScript");
        sol::usertype<MWScriptVariables> mwscriptVars
            = context.mLua->sol().new_usertype<MWScriptVariables>("MWScriptVariables");
        mwscript[sol::meta_function::to_string]
            = [](const MWScriptRef& s) { return std::string("MWScript{") + s.mId.toDebugString() + "}"; };
        mwscript["isRunning"] = sol::readonly_property([](const MWScriptRef& s) { return s.isRunning(); });
        mwscript["recordId"] = sol::readonly_property([](const MWScriptRef& s) { return s.mId.serializeText(); });
        mwscript["variables"] = sol::readonly_property([](const MWScriptRef& s) { return MWScriptVariables{ s }; });
        mwscript["object"] = sol::readonly_property([](const MWScriptRef& s) -> sol::optional<GObject> {
            if (s.mObj)
                return s.mObj;
            const MWScript::GlobalScriptDesc* script
                = MWBase::Environment::get().getScriptManager()->getGlobalScripts().getScriptIfPresent(s.mId);
            if (!script)
                throw std::runtime_error("Invalid MWScriptRef");
            const MWWorld::Ptr* ptr = script->getPtrIfPresent();
            if (ptr && !ptr->isEmpty())
                return GObject(*ptr);
            else
                return sol::nullopt;
        });
        mwscript["player"] = sol::readonly_property(
            [](const MWScriptRef&) { return GObject(MWBase::Environment::get().getWorld()->getPlayerPtr()); });
        mwscriptVars[sol::meta_function::length]
            = [](MWScriptVariables& s) { return s.mRef.getLocals().getSize(s.mRef.mId); };
        mwscriptVars[sol::meta_function::index] = sol::overload(
            [](MWScriptVariables& s, std::string_view var) -> sol::optional<double> {
                if (s.mRef.getLocals().hasVar(s.mRef.mId, var))
                    return s.mRef.getLocals().getVarAsDouble(s.mRef.mId, Misc::StringUtils::lowerCase(var));
                else
                    return sol::nullopt;
            },
            [](MWScriptVariables& s, std::size_t index) -> sol::optional<double> {
                auto& locals = s.mRef.getLocals();
                if (index < 1 || locals.getSize(s.mRef.mId) < index)
                    return sol::nullopt;
                if (index <= locals.mShorts.size())
                    return locals.mShorts[index - 1];
                index -= locals.mShorts.size();
                if (index <= locals.mLongs.size())
                    return locals.mLongs[index - 1];
                index -= locals.mLongs.size();
                if (index <= locals.mFloats.size())
                    return locals.mFloats[index - 1];
                return sol::nullopt;
            });
        mwscriptVars[sol::meta_function::new_index] = sol::overload(
            [](MWScriptVariables& s, std::string_view var, double val) {
                MWScript::Locals& locals = s.mRef.getLocals();
                if (!locals.setVar(s.mRef.mId, Misc::StringUtils::lowerCase(var), val))
                    throw std::runtime_error(
                        "No variable \"" + std::string(var) + "\" in mwscript " + s.mRef.mId.toDebugString());
            },
            [](MWScriptVariables& s, std::size_t index, double val) {
                auto& locals = s.mRef.getLocals();
                if (index < 1 || locals.getSize(s.mRef.mId) < index)
                    throw std::runtime_error("Index out of range in mwscript " + s.mRef.mId.toDebugString());
                if (index <= locals.mShorts.size())
                {
                    locals.mShorts[index - 1] = static_cast<Interpreter::Type_Short>(val);
                    return;
                }
                index -= locals.mShorts.size();
                if (index <= locals.mLongs.size())
                {
                    locals.mLongs[index - 1] = static_cast<Interpreter::Type_Integer>(val);
                    return;
                }
                index -= locals.mLongs.size();
                if (index <= locals.mFloats.size())
                    locals.mFloats[index - 1] = static_cast<Interpreter::Type_Float>(val);
            });
        mwscriptVars[sol::meta_function::pairs] = [](MWScriptVariables& s) {
            std::size_t index = 0;
            const auto& compilerLocals = MWBase::Environment::get().getScriptManager()->getLocals(s.mRef.mId);
            auto& locals = s.mRef.getLocals();
            std::size_t size = locals.getSize(s.mRef.mId);
            return sol::as_function(
                [&, index, size](sol::this_state ts) mutable -> sol::optional<std::tuple<std::string_view, double>> {
                    if (index >= size)
                        return sol::nullopt;
                    auto i = index++;
                    if (i <= locals.mShorts.size())
                        return std::make_tuple<std::string_view, double>(compilerLocals.get('s')[i], locals.mShorts[i]);
                    i -= locals.mShorts.size();
                    if (i <= locals.mLongs.size())
                        return std::make_tuple<std::string_view, double>(compilerLocals.get('l')[i], locals.mLongs[i]);
                    i -= locals.mLongs.size();
                    if (i <= locals.mFloats.size())
                        return std::make_tuple<std::string_view, double>(compilerLocals.get('f')[i], locals.mFloats[i]);
                    return sol::nullopt;
                });
        };

        using GlobalStore = MWWorld::Store<ESM::Global>;
        sol::usertype<GlobalStore> globalStoreT = lua.new_usertype<GlobalStore>("ESM3_GlobalStore");
        const GlobalStore* globalStore = &MWBase::Environment::get().getWorld()->getStore().get<ESM::Global>();
        globalStoreT[sol::meta_function::to_string] = [](const GlobalStore& store) {
            return "ESM3_GlobalStore{" + std::to_string(store.getSize()) + " globals}";
        };
        globalStoreT[sol::meta_function::length] = [](const GlobalStore& store) { return store.getSize(); };
        globalStoreT[sol::meta_function::index] = sol::overload(
            [](const GlobalStore& store, std::string_view globalId) -> sol::optional<float> {
                auto g = store.search(ESM::RefId::deserializeText(globalId));
                if (g == nullptr)
                    return sol::nullopt;
                return getGlobalVariableValue(globalId);
            },
            [](const GlobalStore& store, size_t index) -> sol::optional<float> {
                if (index < 1 || store.getSize() < index)
                    return sol::nullopt;
                auto g = store.at(index - 1);
                if (g == nullptr)
                    return sol::nullopt;
                std::string globalId = g->mId.serializeText();
                return getGlobalVariableValue(globalId);
            });
        globalStoreT[sol::meta_function::new_index] = sol::overload(
            [](const GlobalStore& store, std::string_view globalId, float val) -> void {
                auto g = store.search(ESM::RefId::deserializeText(globalId));
                if (g == nullptr)
                    throw std::runtime_error("No variable \"" + std::string(globalId) + "\" in GlobalStore");
                setGlobalVariableValue(globalId, val);
            },
            [](const GlobalStore& store, size_t index, float val) {
                if (index < 1 || store.getSize() < index)
                    return;
                auto g = store.at(index - 1);
                if (g == nullptr)
                    return;
                std::string globalId = g->mId.serializeText();
                setGlobalVariableValue(globalId, val);
            });
        globalStoreT[sol::meta_function::pairs] = [](const GlobalStore& store) {
            size_t index = 0;
            return sol::as_function(
                [index, &store](sol::this_state ts) mutable -> sol::optional<std::tuple<std::string, float>> {
                    if (index >= store.getSize())
                        return sol::nullopt;

                    const ESM::Global* global = store.at(index++);
                    if (!global)
                        return sol::nullopt;

                    std::string globalId = global->mId.serializeText();
                    float value = getGlobalVariableValue(globalId);

                    return std::make_tuple(globalId, value);
                });
        };
        globalStoreT[sol::meta_function::ipairs] = lua["ipairsForArray"].template get<sol::function>();
        api["getGlobalVariables"] = [globalStore](sol::optional<GObject> player) {
            if (player.has_value() && player->ptr() != MWBase::Environment::get().getWorld()->getPlayerPtr())
                throw std::runtime_error("First argument must either be a player or be missing");

            return globalStore;
        };
        return LuaUtil::makeReadOnly(api);
    }
}
