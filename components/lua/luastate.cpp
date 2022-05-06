#include "luastate.hpp"

#ifndef NO_LUAJIT
#include <luajit.h>
#endif // NO_LUAJIT

#include <filesystem>

#include <components/debug/debuglog.hpp>

namespace LuaUtil
{

    static std::string packageNameToVfsPath(std::string_view packageName, const VFS::Manager* vfs)
    {
        std::string path(packageName);
        std::replace(path.begin(), path.end(), '.', '/');
        std::string pathWithInit = path + "/init.lua";
        path.append(".lua");
        if (vfs->exists(path))
            return path;
        else if (vfs->exists(pathWithInit))
            return pathWithInit;
        else
            throw std::runtime_error("module not found: " + std::string(packageName));
    }

    static std::string packageNameToPath(std::string_view packageName, const std::vector<std::string>& searchDirs)
    {
        std::string path(packageName);
        std::replace(path.begin(), path.end(), '.', '/');
        std::string pathWithInit = path + "/init.lua";
        path.append(".lua");
        for (const std::string& dir : searchDirs)
        {
            std::filesystem::path base(dir);
            std::filesystem::path p1 = base / path;
            if (std::filesystem::exists(p1))
                return p1.string();
            std::filesystem::path p2 = base / pathWithInit;
            if (std::filesystem::exists(p2))
                return p2.string();
        }
        throw std::runtime_error("module not found: " + std::string(packageName));
    }

    static const std::string safeFunctions[] = {
        "assert", "error", "ipairs", "next", "pairs", "pcall", "select", "tonumber", "tostring",
        "type", "unpack", "xpcall", "rawequal", "rawget", "rawset", "setmetatable"};
    static const std::string safePackages[] = {"coroutine", "math", "string", "table"};

    LuaState::LuaState(const VFS::Manager* vfs, const ScriptsConfiguration* conf) : mConf(conf), mVFS(vfs)
    {
        mLua.open_libraries(sol::lib::base, sol::lib::coroutine, sol::lib::math, sol::lib::bit32,
                            sol::lib::string, sol::lib::table, sol::lib::os, sol::lib::debug);

        mLua["math"]["randomseed"](static_cast<unsigned>(std::time(nullptr)));
        mLua["math"]["randomseed"] = []{};

        mLua["writeToLog"] = [](std::string_view s) { Log(Debug::Level::Info) << s; };

        // Some fixes for compatibility between different Lua versions
        if (mLua["unpack"] == sol::nil)
            mLua["unpack"] = mLua["table"]["unpack"];
        else if (mLua["table"]["unpack"] == sol::nil)
            mLua["table"]["unpack"] = mLua["unpack"];
        if (LUA_VERSION_NUM <= 501)
        {
            mLua.script(R"(
                local _pairs = pairs
                local _ipairs = ipairs
                pairs = function(v) return (rawget(getmetatable(v) or {}, '__pairs') or _pairs)(v) end
                ipairs = function(v) return (rawget(getmetatable(v) or {}, '__ipairs') or _ipairs)(v) end
            )");
        }

        mLua.script(R"(
            local printToLog = function(...)
                local strs = {}
                for i = 1, select('#', ...) do
                    strs[i] = tostring(select(i, ...))
                end
                return writeToLog(table.concat(strs, '\t'))
            end
            printGen = function(name) return function(...) return printToLog(name, ...) end end

            function pairsForReadOnly(v)
                local nextFn, t, firstKey = pairs(getmetatable(v).__index)
                return function(_, k) return nextFn(t, k) end, v, firstKey
            end
            function ipairsForReadOnly(v)
                local nextFn, t, firstKey = ipairs(getmetatable(v).__index)
                return function(_, k) return nextFn(t, k) end, v, firstKey
            end
            local function nextForArray(array, index)
                index = (index or 0) + 1
                if index < #array then
                    return index, array[index]
                end
            end
            function ipairsForArray(array)
                return nextForArray, array, 0
            end

            getmetatable('').__metatable = false
            getSafeMetatable = function(v)
                if type(v) ~= 'table' then error('getmetatable is allowed only for tables', 2) end
                return getmetatable(v)
            end
        )");

        mSandboxEnv = sol::table(mLua, sol::create);
        mSandboxEnv["_VERSION"] = mLua["_VERSION"];
        for (const std::string& s : safeFunctions)
        {
            if (mLua[s] == sol::nil) throw std::logic_error("Lua function not found: " + s);
            mSandboxEnv[s] = mLua[s];
        }
        for (const std::string& s : safePackages)
        {
            if (mLua[s] == sol::nil) throw std::logic_error("Lua package not found: " + s);
            mCommonPackages[s] = mSandboxEnv[s] = makeReadOnly(mLua[s]);
        }
        mSandboxEnv["getmetatable"] = mLua["getSafeMetatable"];
        mCommonPackages["os"] = mSandboxEnv["os"] = makeReadOnly(tableFromPairs<std::string_view, sol::function>({
            {"date", mLua["os"]["date"]},
            {"difftime", mLua["os"]["difftime"]},
            {"time", mLua["os"]["time"]}
        }));
    }

    LuaState::~LuaState()
    {
        // Should be cleaned before destructing mLua.
        mCommonPackages.clear();
        mSandboxEnv = sol::nil;
    }

    sol::table makeReadOnly(sol::table table)
    {
        if (table == sol::nil)
            return table;
        if (table.is<sol::userdata>())
            return table;  // it is already userdata, no sense to wrap it again

        lua_State* luaState = table.lua_state();
        sol::state_view lua(luaState);
        sol::table meta(lua, sol::create);
        meta["__index"] = table;
        meta["__pairs"] = lua["pairsForReadOnly"];
        meta["__ipairs"] = lua["ipairsForReadOnly"];

        lua_newuserdata(luaState, 0);
        sol::stack::push(luaState, meta);
        lua_setmetatable(luaState, -2);
        return sol::stack::pop<sol::table>(luaState);
    }

    sol::table getMutableFromReadOnly(const sol::userdata& ro)
    {
        return ro[sol::metatable_key].get<sol::table>()["__index"];
    }

    void LuaState::addCommonPackage(std::string packageName, sol::object package)
    {
        if (!package.is<sol::function>())
            package = makeReadOnly(std::move(package));
        mCommonPackages.emplace(std::move(packageName), std::move(package));
    }

    sol::protected_function_result LuaState::runInNewSandbox(
        const std::string& path, const std::string& namePrefix,
        const std::map<std::string, sol::object>& packages, const sol::object& hiddenData)
    {
        sol::protected_function script = loadScriptAndCache(path);

        sol::environment env(mLua, sol::create, mSandboxEnv);
        std::string envName = namePrefix + "[" + path + "]:";
        env["print"] = mLua["printGen"](envName);
        env["_G"] = env;
        env[sol::metatable_key]["__metatable"] = false;

        auto maybeRunLoader = [&hiddenData](const sol::object& package) -> sol::object
        {
            if (package.is<sol::function>())
                return call(package.as<sol::function>(), hiddenData);
            else
                return package;
        };
        sol::table loaded(mLua, sol::create);
        for (const auto& [key, value] : mCommonPackages)
            loaded[key] = maybeRunLoader(value);
        for (const auto& [key, value] : packages)
            loaded[key] = maybeRunLoader(value);
        env["require"] = [this, env, loaded, hiddenData](std::string_view packageName) mutable
        {
            sol::object package = loaded[packageName];
            if (package != sol::nil)
                return package;
            sol::protected_function packageLoader = loadScriptAndCache(packageNameToVfsPath(packageName, mVFS));
            sol::set_environment(env, packageLoader);
            package = call(packageLoader, packageName);
            loaded[packageName] = package;
            return package;
        };

        sol::set_environment(env, script);
        return call(script);
    }

    sol::environment LuaState::newInternalLibEnvironment()
    {
        sol::environment env(mLua, sol::create, mSandboxEnv);
        sol::table loaded(mLua, sol::create);
        for (const std::string& s : safePackages)
            loaded[s] = static_cast<sol::object>(mSandboxEnv[s]);
        env["require"] = [this, loaded, env](const std::string& module) mutable
        {
            if (loaded[module] != sol::nil)
                return loaded[module];
            sol::protected_function initializer = loadInternalLib(module);
            sol::set_environment(env, initializer);
            loaded[module] = call(initializer, module);
            return loaded[module];
        };
        return env;
    }

    sol::protected_function_result LuaState::throwIfError(sol::protected_function_result&& res)
    {
        if (!res.valid() && static_cast<int>(res.get_type()) == LUA_TSTRING)
            throw std::runtime_error("Lua error: " + res.get<std::string>());
        else
            return std::move(res);
    }

    sol::function LuaState::loadScriptAndCache(const std::string& path)
    {
        auto iter = mCompiledScripts.find(path);
        if (iter != mCompiledScripts.end())
            return mLua.load(iter->second.as_string_view(), path, sol::load_mode::binary);
        sol::function res = loadFromVFS(path);
        mCompiledScripts[path] = res.dump();
        return res;
    }

    sol::function LuaState::loadFromVFS(const std::string& path)
    {
        std::string fileContent(std::istreambuf_iterator<char>(*mVFS->get(path)), {});
        sol::load_result res = mLua.load(fileContent, path, sol::load_mode::text);
        if (!res.valid())
            throw std::runtime_error("Lua error: " + res.get<std::string>());
        return res;
    }

    sol::function LuaState::loadInternalLib(std::string_view libName)
    {
        std::string path = packageNameToPath(libName, mLibSearchPaths);
        sol::load_result res = mLua.load_file(path, sol::load_mode::text);
        if (!res.valid())
            throw std::runtime_error("Lua error: " + res.get<std::string>());
        return res;
    }

    std::string getLuaVersion()
    {
        #ifdef NO_LUAJIT
        return LUA_RELEASE;
        #else
        return LUA_RELEASE " (" LUAJIT_VERSION ")";
        #endif
    }

    std::string toString(const sol::object& obj)
    {
        if (obj == sol::nil)
            return "nil";
        else if (obj.get_type() == sol::type::string)
            return "\"" + obj.as<std::string>() + "\"";
        else
            return call(sol::state_view(obj.lua_state())["tostring"], obj);
    }

}
