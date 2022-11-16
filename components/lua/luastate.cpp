#include "luastate.hpp"

#ifndef NO_LUAJIT
#include <luajit.h>
#endif // NO_LUAJIT

#include <algorithm>
#include <ctime>
#include <filesystem>
#include <istream>
#include <iterator>
#include <memory>
#include <type_traits>

#include <components/debug/debuglog.hpp>
#include <components/files/conversion.hpp>
#include <components/vfs/manager.hpp>

#include "scriptscontainer.hpp"

namespace LuaUtil
{
    class ScriptsConfiguration;

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

    static std::filesystem::path packageNameToPath(
        std::string_view packageName, const std::vector<std::filesystem::path>& searchDirs)
    {
        std::string path(packageName);
        std::replace(path.begin(), path.end(), '.', '/');
        std::string pathWithInit = path + "/init.lua";
        path.append(".lua");
        for (const auto& base : searchDirs)
        {
            std::filesystem::path p1 = base / path;
            if (std::filesystem::exists(p1))
                return p1;
            std::filesystem::path p2 = base / pathWithInit;
            if (std::filesystem::exists(p2))
                return p2;
        }
        throw std::runtime_error("module not found: " + std::string(packageName));
    }

    static const std::string safeFunctions[] = { "assert", "error", "ipairs", "next", "pairs", "pcall", "select",
        "tonumber", "tostring", "type", "unpack", "xpcall", "rawequal", "rawget", "rawset", "setmetatable" };
    static const std::string safePackages[] = { "coroutine", "math", "string", "table" };

    static constexpr int64_t countHookStep = 1000;

    bool LuaState::sProfilerEnabled = true;

    void LuaState::countHook(lua_State* L, lua_Debug* ar)
    {
        LuaState* self;
        (void)lua_getallocf(L, reinterpret_cast<void**>(&self));
        if (self->mActiveScriptIdStack.empty())
            return;
        const ScriptId& activeScript = self->mActiveScriptIdStack.back();
        activeScript.mContainer->addInstructionCount(activeScript.mIndex, countHookStep);
        self->mWatchdogInstructionCounter += countHookStep;
        if (self->mSettings.mInstructionLimit > 0
            && self->mWatchdogInstructionCounter > self->mSettings.mInstructionLimit)
        {
            lua_pushstring(L,
                "Lua instruction count exceeded, probably an infinite loop in a script. "
                "To change the limit set \"[Lua] instruction limit per call\" in settings.cfg");
            lua_error(L);
        }
    }

    void* LuaState::trackingAllocator(void* ud, void* ptr, size_t osize, size_t nsize)
    {
        LuaState* self = static_cast<LuaState*>(ud);
        const uint64_t smallAllocSize = self->mSettings.mSmallAllocMaxSize;
        const uint64_t memoryLimit = self->mSettings.mMemoryLimit;

        if (!ptr)
            osize = 0;
        int64_t smallAllocDelta = 0, bigAllocDelta = 0;
        if (osize <= smallAllocSize)
            smallAllocDelta -= osize;
        else
            bigAllocDelta -= osize;
        if (nsize <= smallAllocSize)
            smallAllocDelta += nsize;
        else
            bigAllocDelta += nsize;

        if (bigAllocDelta > 0 && memoryLimit > 0 && self->mTotalMemoryUsage + nsize - osize > memoryLimit)
        {
            Log(Debug::Error) << "Lua realloc " << osize << "->" << nsize
                              << " is blocked because Lua memory limit (configurable in settings.cfg) is exceeded";
            return nullptr;
        }
        self->mTotalMemoryUsage += smallAllocDelta + bigAllocDelta;
        self->mSmallAllocMemoryUsage += smallAllocDelta;

        void* newPtr = nullptr;
        if (nsize == 0)
            free(ptr);
        else
            newPtr = realloc(ptr, nsize);

        if (bigAllocDelta != 0)
        {
            auto it = osize > smallAllocSize ? self->mBigAllocOwners.find(ptr) : self->mBigAllocOwners.end();
            ScriptId id;
            if (it != self->mBigAllocOwners.end())
            {
                if (it->second.mContainer)
                    id = ScriptId{ *it->second.mContainer, it->second.mScriptIndex };
                if (ptr != newPtr || nsize <= smallAllocSize)
                    self->mBigAllocOwners.erase(it);
            }
            else if (bigAllocDelta > 0)
            {
                if (!self->mActiveScriptIdStack.empty())
                    id = self->mActiveScriptIdStack.back();
                bigAllocDelta = nsize;
            }
            if (id.mContainer)
            {
                if (static_cast<size_t>(id.mIndex) >= self->mMemoryUsage.size())
                    self->mMemoryUsage.resize(id.mIndex + 1);
                self->mMemoryUsage[id.mIndex] += bigAllocDelta;
                id.mContainer->addMemoryUsage(id.mIndex, bigAllocDelta);
                if (newPtr && nsize > smallAllocSize)
                    self->mBigAllocOwners.emplace(newPtr, AllocOwner{ id.mContainer->mThis, id.mIndex });
            }
        }

        return newPtr;
    }

    lua_State* LuaState::createLuaRuntime(LuaState* luaState)
    {
        if (sProfilerEnabled)
        {
            Log(Debug::Info) << "Initializing LuaUtil::LuaState with profiler";
            lua_State* L = lua_newstate(&trackingAllocator, luaState);
            if (L)
                return L;
            else
            {
                sProfilerEnabled = false;
                Log(Debug::Error)
                    << "Failed to initialize LuaUtil::LuaState with custom allocator; disabling Lua profiler";
            }
        }
        Log(Debug::Info) << "Initializing LuaUtil::LuaState without profiler";
        lua_State* L = luaL_newstate();
        if (!L)
            throw std::runtime_error("Can't create Lua runtime");
        return L;
    }

    LuaState::LuaState(const VFS::Manager* vfs, const ScriptsConfiguration* conf, const LuaStateSettings& settings)
        : mSettings(settings)
        , mLuaHolder(createLuaRuntime(this))
        , mSol(mLuaHolder.get())
        , mConf(conf)
        , mVFS(vfs)
    {
        if (sProfilerEnabled)
            lua_sethook(mLuaHolder.get(), &countHook, LUA_MASKCOUNT, countHookStep);

        mSol.open_libraries(sol::lib::base, sol::lib::coroutine, sol::lib::math, sol::lib::bit32, sol::lib::string,
            sol::lib::table, sol::lib::os, sol::lib::debug);

        mSol["math"]["randomseed"](static_cast<unsigned>(std::time(nullptr)));
        mSol["math"]["randomseed"] = [] {};

        mSol["writeToLog"] = [](std::string_view s) { Log(Debug::Level::Info) << s; };

        // Some fixes for compatibility between different Lua versions
        if (mSol["unpack"] == sol::nil)
            mSol["unpack"] = mSol["table"]["unpack"];
        else if (mSol["table"]["unpack"] == sol::nil)
            mSol["table"]["unpack"] = mSol["unpack"];
        if (LUA_VERSION_NUM <= 501)
        {
            mSol.script(R"(
                local _pairs = pairs
                local _ipairs = ipairs
                pairs = function(v) return (rawget(getmetatable(v) or {}, '__pairs') or _pairs)(v) end
                ipairs = function(v) return (rawget(getmetatable(v) or {}, '__ipairs') or _ipairs)(v) end
            )");
        }

        mSol.script(R"(
            local printToLog = function(...)
                local strs = {}
                for i = 1, select('#', ...) do
                    strs[i] = tostring(select(i, ...))
                end
                return writeToLog(table.concat(strs, '\t'))
            end
            printGen = function(name) return function(...) return printToLog(name, ...) end end

            function createStrictIndexFn(tbl)
                return function(_, key)
                    local res = tbl[key]
                    if res ~= nil then
                        return res
                    else
                        error('Key not found: '..tostring(key), 2)
                    end
                end
            end
            function pairsForReadOnly(v)
                local nextFn, t, firstKey = pairs(getmetatable(v).t)
                return function(_, k) return nextFn(t, k) end, v, firstKey
            end
            function ipairsForReadOnly(v)
                local nextFn, t, firstKey = ipairs(getmetatable(v).t)
                return function(_, k) return nextFn(t, k) end, v, firstKey
            end
            local function nextForArray(array, index)
                index = (index or 0) + 1
                if index <= #array then
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

        mSandboxEnv = sol::table(mSol, sol::create);
        mSandboxEnv["_VERSION"] = mSol["_VERSION"];
        for (const std::string& s : safeFunctions)
        {
            if (mSol[s] == sol::nil)
                throw std::logic_error("Lua function not found: " + s);
            mSandboxEnv[s] = mSol[s];
        }
        for (const std::string& s : safePackages)
        {
            if (mSol[s] == sol::nil)
                throw std::logic_error("Lua package not found: " + s);
            mCommonPackages[s] = mSandboxEnv[s] = makeReadOnly(mSol[s]);
        }
        mSandboxEnv["getmetatable"] = mSol["getSafeMetatable"];
        mCommonPackages["os"] = mSandboxEnv["os"]
            = makeReadOnly(tableFromPairs<std::string_view, sol::function>({ { "date", mSol["os"]["date"] },
                { "difftime", mSol["os"]["difftime"] }, { "time", mSol["os"]["time"] } }));
    }

    sol::table makeReadOnly(const sol::table& table, bool strictIndex)
    {
        if (table == sol::nil)
            return table;
        if (table.is<sol::userdata>())
            return table; // it is already userdata, no sense to wrap it again

        lua_State* luaState = table.lua_state();
        sol::state_view lua(luaState);
        sol::table meta(lua, sol::create);
        meta["t"] = table;
        if (strictIndex)
            meta["__index"] = lua["createStrictIndexFn"](table);
        else
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
        return ro[sol::metatable_key].get<sol::table>()["t"];
    }

    void LuaState::addCommonPackage(std::string packageName, sol::object package)
    {
        if (!package.is<sol::function>())
            package = makeReadOnly(std::move(package));
        mCommonPackages.emplace(std::move(packageName), std::move(package));
    }

    sol::protected_function_result LuaState::runInNewSandbox(const std::string& path, const std::string& namePrefix,
        const std::map<std::string, sol::object>& packages, const sol::object& hiddenData)
    {
        sol::protected_function script = loadScriptAndCache(path);

        sol::environment env(mSol, sol::create, mSandboxEnv);
        std::string envName = namePrefix + "[" + path + "]:";
        env["print"] = mSol["printGen"](envName);
        env["_G"] = env;
        env[sol::metatable_key]["__metatable"] = false;

        ScriptId scriptId;
        if (hiddenData.is<sol::table>())
            scriptId = hiddenData.as<sol::table>()
                           .get<sol::optional<ScriptId>>(ScriptsContainer::sScriptIdKey)
                           .value_or(ScriptId{});

        auto maybeRunLoader = [&hiddenData, scriptId](const sol::object& package) -> sol::object {
            if (package.is<sol::function>())
                return call(scriptId, package.as<sol::function>(), hiddenData);
            else
                return package;
        };
        sol::table loaded(mSol, sol::create);
        for (const auto& [key, value] : mCommonPackages)
            loaded[key] = maybeRunLoader(value);
        for (const auto& [key, value] : packages)
            loaded[key] = maybeRunLoader(value);
        env["require"] = [this, env, loaded, hiddenData](std::string_view packageName) mutable {
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
        return call(scriptId, script);
    }

    sol::environment LuaState::newInternalLibEnvironment()
    {
        sol::environment env(mSol, sol::create, mSandboxEnv);
        sol::table loaded(mSol, sol::create);
        for (const std::string& s : safePackages)
            loaded[s] = static_cast<sol::object>(mSandboxEnv[s]);
        env["require"] = [this, loaded, env](const std::string& module) mutable {
            if (loaded[module] != sol::nil)
                return loaded[module];
            sol::protected_function initializer = loadInternalLib(module);
            sol::set_environment(env, initializer);
            loaded[module] = call({}, initializer, module);
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
            return mSol.load(iter->second.as_string_view(), path, sol::load_mode::binary);
        sol::function res = loadFromVFS(path);
        mCompiledScripts[path] = res.dump();
        return res;
    }

    sol::function LuaState::loadFromVFS(const std::string& path)
    {
        std::string fileContent(std::istreambuf_iterator<char>(*mVFS->get(path)), {});
        sol::load_result res = mSol.load(fileContent, path, sol::load_mode::text);
        if (!res.valid())
            throw std::runtime_error("Lua error: " + res.get<std::string>());
        return res;
    }

    sol::function LuaState::loadInternalLib(std::string_view libName)
    {
        const auto path = packageNameToPath(libName, mLibSearchPaths);
        sol::load_result res = mSol.load_file(Files::pathToUnicodeString(path), sol::load_mode::text);
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
