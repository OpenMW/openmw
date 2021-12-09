#ifndef COMPONENTS_LUA_LUASTATE_H
#define COMPONENTS_LUA_LUASTATE_H

#include <map>

#include <sol/sol.hpp>

#include <components/vfs/manager.hpp>

#include "configuration.hpp"

namespace LuaUtil
{

    std::string getLuaVersion();

    // Holds Lua state.
    // Provides additional features:
    //   - Load scripts from the virtual filesystem;
    //   - Caching of loaded scripts;
    //   - Disable unsafe Lua functions;
    //   - Run every instance of every script in a separate sandbox;
    //   - Forbid any interactions between sandboxes except than via provided API;
    //   - Access to common read-only resources from different sandboxes;
    //   - Replace standard `require` with a safe version that allows to search
    //         Lua libraries (only source, no dll's) in the virtual filesystem;
    //   - Make `print` to add the script name to every message and
    //         write to the Log rather than directly to stdout;
    class LuaState
    {
    public:
        explicit LuaState(const VFS::Manager* vfs, const ScriptsConfiguration* conf);
        ~LuaState();

        // Returns underlying sol::state.
        sol::state& sol() { return mLua; }

        // Can be used by a C++ function that is called from Lua to get the Lua traceback.
        // Makes no sense if called not from Lua code.
        // Note: It is a slow function, should be used for debug purposes only.
        std::string debugTraceback() { return mLua["debug"]["traceback"]().get<std::string>(); }

        // A shortcut to create a new Lua table.
        sol::table newTable() { return sol::table(mLua, sol::create); }

        template <typename Key, typename Value>
        sol::table tableFromPairs(std::initializer_list<std::pair<Key, Value>> list)
        {
            sol::table res(mLua, sol::create);
            for (const auto& [k, v] : list)
                res[k] = v;
            return res;
        }

        // Registers a package that will be available from every sandbox via `require(name)`.
        // The package can be either a sol::table with an API or a sol::function. If it is a function,
        // it will be evaluated (once per sandbox) the first time when requested. If the package
        // is a table, then `makeReadOnly` is applied to it automatically (but not to other tables it contains).
        void addCommonPackage(std::string packageName, sol::object package);

        // Creates a new sandbox, runs a script, and returns the result
        // (the result is expected to be an interface of the script).
        // Args:
        //     path: path to the script in the virtual filesystem;
        //     namePrefix: sandbox name will be "<namePrefix>[<filePath>]". Sandbox name
        //         will be added to every `print` output.
        //     packages: additional packages that should be available from the sandbox via `require`. Each package
        //         should be either a sol::table or a sol::function. If it is a function, it will be evaluated
        //         (once per sandbox) with the argument 'hiddenData' the first time when requested.
        sol::protected_function_result runInNewSandbox(const std::string& path,
                                                       const std::string& namePrefix = "",
                                                       const std::map<std::string, sol::object>& packages = {},
                                                       const sol::object& hiddenData = sol::nil);

        void dropScriptCache() { mCompiledScripts.clear(); }

        const ScriptsConfiguration& getConfiguration() const { return *mConf; }

    private:
        static sol::protected_function_result throwIfError(sol::protected_function_result&&);
        template <typename... Args>
        friend sol::protected_function_result call(const sol::protected_function& fn, Args&&... args);

        sol::function loadScript(const std::string& path);

        sol::state mLua;
        const ScriptsConfiguration* mConf;
        sol::table mSandboxEnv;
        std::map<std::string, sol::bytecode> mCompiledScripts;
        std::map<std::string, sol::object> mCommonPackages;
        const VFS::Manager* mVFS;
    };

    // Should be used for every call of every Lua function.
    // It is a workaround for a bug in `sol`. See https://github.com/ThePhD/sol2/issues/1078
    template <typename... Args>
    sol::protected_function_result call(const sol::protected_function& fn, Args&&... args)
    {
        try
        {
            return LuaState::throwIfError(fn(std::forward<Args>(args)...));
        }
        catch (std::exception&) { throw; }
        catch (...) { throw std::runtime_error("Unknown error"); }
    }

    // getFieldOrNil(table, "a", "b", "c") returns table["a"]["b"]["c"] or nil if some of the fields doesn't exist.
    template <class... Str>
    sol::object getFieldOrNil(const sol::object& table, std::string_view first, const Str&... str)
    {
        if (!table.is<sol::table>())
            return sol::nil;
        if constexpr (sizeof...(str) == 0)
            return table.as<sol::table>()[first];
        else
            return getFieldOrNil(table.as<sol::table>()[first], str...);
    }

    // String representation of a Lua object. Should be used for debugging/logging purposes only.
    std::string toString(const sol::object&);

    // Makes a table read only (when accessed from Lua) by wrapping it with an empty userdata.
    // Needed to forbid any changes in common resources that can be accessed from different sandboxes.
    sol::table makeReadOnly(sol::table);
    sol::table getMutableFromReadOnly(const sol::userdata&);

}

#endif // COMPONENTS_LUA_LUASTATE_H
