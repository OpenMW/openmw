#ifndef COMPONENTS_LUA_LUASTATE_H
#define COMPONENTS_LUA_LUASTATE_H

#include <filesystem>
#include <map>
#include <typeinfo>

#include <sol/sol.hpp>

#include <components/vfs/pathutil.hpp>

#include "configuration.hpp"
#include "luastateptr.hpp"

namespace VFS
{
    class Manager;
}

namespace LuaUtil
{

    std::string getLuaVersion();

    class ScriptsContainer;
    struct ScriptId
    {
        ScriptsContainer* mContainer = nullptr;
        int mIndex = -1; // index in LuaUtil::ScriptsConfiguration
    };

    struct LuaStateSettings
    {
        uint64_t mInstructionLimit = 0; // 0 is unlimited
        uint64_t mMemoryLimit = 0; // 0 is unlimited
        uint64_t mSmallAllocMaxSize = 1024 * 1024; // big default value efficiently disables memory tracking
        bool mLogMemoryUsage = false;
    };

    class LuaState;
    class LuaView
    {
        sol::state_view mSol;

        LuaView(const LuaView&) = delete;

        LuaView(lua_State* state)
            : mSol(state)
        {
        }

    public:
        friend class LuaState;
        // Returns underlying sol::state.
        sol::state_view& sol() { return mSol; }

        // A shortcut to create a new Lua table.
        sol::table newTable() { return sol::table(mSol, sol::create); }
    };

    template <typename Key, typename Value>
    sol::table tableFromPairs(lua_State* state, std::initializer_list<std::pair<Key, Value>> list)
    {
        sol::table res(state, sol::create);
        for (const auto& [k, v] : list)
            res[k] = v;
        return res;
    }

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
        explicit LuaState(const VFS::Manager* vfs, const ScriptsConfiguration* conf,
            const LuaStateSettings& settings = LuaStateSettings{});
        LuaState(const LuaState&) = delete;
        LuaState(LuaState&&) = delete;

        // Pushing to the stack from outside a Lua context crashes the engine if no memory can be allocated to grow the
        // stack
        template <class Function>
        [[nodiscard]] int invokeProtectedCall(Function&& function) const
        {
            if (!lua_checkstack(mSol.lua_state(), 2))
                return LUA_ERRMEM;
            lua_pushcfunction(mSol.lua_state(), [](lua_State* state) {
                void* f = lua_touserdata(state, 1);
                LuaView view(state);
                (*static_cast<Function*>(f))(view);
                return 0;
            });
            lua_pushlightuserdata(mSol.lua_state(), &function);
            return lua_pcall(mSol.lua_state(), 1, 0, 0);
        }

        template <class Lambda>
        void protectedCall(Lambda&& f) const
        {
            int result = invokeProtectedCall(std::forward<Lambda>(f));
            switch (result)
            {
                case LUA_OK:
                    break;
                case LUA_ERRMEM:
                    throw std::runtime_error("Lua error: out of memory");
                case LUA_ERRRUN:
                {
                    sol::optional<std::string> error = sol::stack::check_get<std::string>(mSol.lua_state());
                    if (error)
                        throw std::runtime_error(*error);
                }
                    [[fallthrough]];
                default:
                    throw std::runtime_error("Lua error: " + std::to_string(result));
            }
        }

        // Note that constructing a sol::state_view is only safe from a Lua context. Use protectedCall to get one
        lua_State* unsafeState() const { return mSol.lua_state(); }

        // Can be used by a C++ function that is called from Lua to get the Lua traceback.
        // Makes no sense if called not from Lua code.
        // Note: It is a slow function, should be used for debug purposes only.
        std::string debugTraceback() { return mSol["debug"]["traceback"]().get<std::string>(); }

        // Registers a package that will be available from every sandbox via `require(name)`.
        // The package can be either a sol::table with an API or a sol::function. If it is a function,
        // it will be evaluated (once per sandbox) the first time when requested. If the package
        // is a table, then `makeReadOnly` is applied to it automatically (but not to other tables it contains).
        void addCommonPackage(std::string packageName, sol::object package);

        // Creates a new sandbox, runs a script, and returns the result
        // (the result is expected to be an interface of the script).
        // Args:
        //     path: path to the script in the virtual filesystem;
        //     envName: sandbox name.
        //     packages: additional packages that should be available from the sandbox via `require`. Each package
        //         should be either a sol::table or a sol::function. If it is a function, it will be evaluated
        //         (once per sandbox) with the argument 'hiddenData' the first time when requested.
        sol::protected_function_result runInNewSandbox(const VFS::Path::Normalized& path,
            const std::string& envName = "unnamed", const std::map<std::string, sol::main_object>& packages = {},
            const sol::main_object& hiddenData = sol::nil);

        void dropScriptCache() { mCompiledScripts.clear(); }

        const ScriptsConfiguration& getConfiguration() const { return *mConf; }

        // Load internal Lua library. All libraries are loaded in one sandbox and shouldn't be exposed to scripts
        // directly.
        void addInternalLibSearchPath(const std::filesystem::path& path) { mLibSearchPaths.push_back(path); }
        sol::function loadInternalLib(std::string_view libName);
        sol::function loadFromVFS(const VFS::Path::Normalized& path);
        sol::environment newInternalLibEnvironment();

        uint64_t getTotalMemoryUsage() const { return mSol.memory_used(); }
        uint64_t getSmallAllocMemoryUsage() const { return mSmallAllocMemoryUsage; }
        uint64_t getMemoryUsageByScriptIndex(unsigned id) const
        {
            return id < mMemoryUsage.size() ? mMemoryUsage[id] : 0;
        }

        const LuaStateSettings& getSettings() const { return mSettings; }

        // Note: Lua profiler can not be re-enabled after disabling.
        static void disableProfiler() { sProfilerEnabled = false; }
        static bool isProfilerEnabled() { return sProfilerEnabled; }

        static sol::protected_function_result throwIfError(sol::protected_function_result&&);

    private:
        template <typename... Args>
        friend sol::protected_function_result call(const sol::protected_function& fn, Args&&... args);
        template <typename... Args>
        friend sol::protected_function_result call(
            ScriptId scriptId, const sol::protected_function& fn, Args&&... args);

        sol::function loadScriptAndCache(const VFS::Path::Normalized& path);
        static void countHook(lua_State* state, lua_Debug* ar);
        static void* trackingAllocator(void* ud, void* ptr, size_t osize, size_t nsize);

        static LuaStatePtr createLuaRuntime(LuaState* luaState);

        struct AllocOwner
        {
            std::shared_ptr<ScriptsContainer*> mContainer;
            int mScriptIndex;
        };

        const LuaStateSettings mSettings;

        // Needed to track resource usage per script, must be initialized before mLuaHolder.
        std::vector<ScriptId> mActiveScriptIdStack;
        uint64_t mWatchdogInstructionCounter = 0;
        std::map<void*, AllocOwner> mBigAllocOwners;
        uint64_t mTotalMemoryUsage = 0;
        uint64_t mSmallAllocMemoryUsage = 0;
        std::vector<int64_t> mMemoryUsage;

        // Must be declared before mSol and all sol-related objects. Then on exit it will be destructed the last.
        LuaStatePtr mLuaState;

        sol::state_view mSol;
        const ScriptsConfiguration* mConf;
        sol::table mSandboxEnv;
        std::map<VFS::Path::Normalized, sol::bytecode> mCompiledScripts;
        std::map<std::string, sol::object> mCommonPackages;
        const VFS::Manager* mVFS;
        std::vector<std::filesystem::path> mLibSearchPaths;

        static bool sProfilerEnabled;
    };

    // LuaUtil::call should be used for every call of every Lua function.
    // 1) It is a workaround for a bug in `sol`. See https://github.com/ThePhD/sol2/issues/1078
    // 2) When called with ScriptId it tracks resource usage (scriptId refers to the script that is responsible for this
    // call).

    template <typename... Args>
    sol::protected_function_result call(const sol::protected_function& fn, Args&&... args)
    {
        try
        {
            auto res = LuaState::throwIfError(fn(std::forward<Args>(args)...));
            return res;
        }
        catch (std::exception&)
        {
            throw;
        }
        catch (...)
        {
            throw std::runtime_error("Unknown error");
        }
    }

    // Lua must be initialized through LuaUtil::LuaState, otherwise this function will segfault.
    template <typename... Args>
    sol::protected_function_result call(ScriptId scriptId, const sol::protected_function& fn, Args&&... args)
    {
        LuaState* luaState = nullptr;
        if (LuaState::sProfilerEnabled && scriptId.mContainer)
        {
            (void)lua_getallocf(fn.lua_state(), reinterpret_cast<void**>(&luaState));
            luaState->mActiveScriptIdStack.push_back(scriptId);
            luaState->mWatchdogInstructionCounter = 0;
        }
        try
        {
            auto res = LuaState::throwIfError(fn(std::forward<Args>(args)...));
            if (luaState)
                luaState->mActiveScriptIdStack.pop_back();
            return res;
        }
        catch (std::exception&)
        {
            if (luaState)
                luaState->mActiveScriptIdStack.pop_back();
            throw;
        }
        catch (...)
        {
            if (luaState)
                luaState->mActiveScriptIdStack.pop_back();
            throw std::runtime_error("Unknown error");
        }
    }

    // work around for a (likely) sol3 bug
    // when the index meta method throws, simply calling table.get crashes instead of re-throwing the error
    template <class Key>
    sol::object safeGet(const sol::table& table, const Key& key)
    {
        auto index = table.traverse_raw_get<sol::optional<sol::main_protected_function>>(
            sol::metatable_key, sol::meta_function::index);
        if (index)
        {
            sol::protected_function_result result = index.value()(table, key);
            if (result.valid())
                return result.get<sol::object>();
            else
                throw result.get<sol::error>();
        }
        else
            return table.raw_get<sol::object>(key);
    }

    // getFieldOrNil(table, "a", "b", "c") returns table["a"]["b"]["c"] or nil if some of the fields doesn't exist.
    template <class... Str>
    sol::object getFieldOrNil(const sol::object& table, std::string_view first, const Str&... str)
    {
        if (!table.is<sol::table>())
            return sol::nil;
        sol::object value = safeGet(table.as<sol::table>(), first);
        if constexpr (sizeof...(str) == 0)
            return value;
        else
            return getFieldOrNil(value, str...);
    }

    template <class... Str>
    void setDeepField(sol::table& table, const sol::object& value, std::string_view first, const Str&... str)
    {
        if constexpr (sizeof...(str) == 0)
            table[first] = value;
        else
        {
            if (table[first] == sol::nil)
                table[first] = sol::table(table.lua_state(), sol::create);
            sol::table nextTable = table[first];
            setDeepField(nextTable, value, str...);
        }
    }

    // String representation of a Lua object. Should be used for debugging/logging purposes only.
    std::string toString(const sol::object&);

    namespace Internal
    {
        std::string formatCastingError(const sol::object& obj, const std::type_info&);
    }

    template <class T>
    decltype(auto) cast(const sol::object& obj)
    {
        if (!obj.is<T>())
            throw std::runtime_error(Internal::formatCastingError(obj, typeid(T)));
        return obj.as<T>();
    }

    template <class T>
    T getValueOrDefault(const sol::object& obj, const T& defaultValue)
    {
        if (obj == sol::nil)
            return defaultValue;
        return cast<T>(obj);
    }

    // Makes a table read only (when accessed from Lua) by wrapping it with an empty userdata.
    // Needed to forbid any changes in common resources that can be accessed from different sandboxes.
    // `strictIndex = true` replaces default `__index` with a strict version that throws an error if key is not found.
    sol::table makeReadOnly(const sol::table&, bool strictIndex = false);
    inline sol::table makeStrictReadOnly(const sol::table& tbl)
    {
        return makeReadOnly(tbl, true);
    }
    sol::table getMutableFromReadOnly(const sol::userdata&);

    template <class T>
    void copyVectorToTable(const std::vector<T>& v, sol::table& out)
    {
        for (const T& t : v)
            out.add(t);
    }
}

#endif // COMPONENTS_LUA_LUASTATE_H
