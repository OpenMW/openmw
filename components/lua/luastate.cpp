#include "luastate.hpp"

#ifndef NO_LUAJIT
#include <luajit.h>
#endif // NO_LUAJIT

#include <filesystem>
#include <fstream>

#include <components/debug/debuglog.hpp>
#include <components/files/conversion.hpp>
#include <components/vfs/manager.hpp>

#include "scriptscontainer.hpp"
#include "utf8.hpp"

namespace LuaUtil
{
    static VFS::Path::Normalized packageNameToVfsPath(std::string_view packageName, const VFS::Manager& vfs)
    {
        std::string pathValue(packageName);
        std::replace(pathValue.begin(), pathValue.end(), '.', '/');
        VFS::Path::Normalized pathWithInit(pathValue + "/init.lua");
        pathValue.append(".lua");
        VFS::Path::Normalized path(pathValue);
        if (vfs.exists(path))
            return path;
        else if (vfs.exists(pathWithInit))
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
    static const std::string safePackages[] = { "coroutine", "math", "string", "table", "utf8" };

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

        void* newPtr = nullptr;
        if (nsize == 0)
            free(ptr);
        else
        {
            newPtr = realloc(ptr, nsize);
            if (!newPtr)
            {
                Log(Debug::Error) << "Lua realloc " << osize << "->" << nsize << " failed";
                return nullptr;
            }
        }
        self->mTotalMemoryUsage += smallAllocDelta + bigAllocDelta;
        self->mSmallAllocMemoryUsage += smallAllocDelta;

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
            if (id.mIndex >= 0)
            {
                if (static_cast<size_t>(id.mIndex) >= self->mMemoryUsage.size())
                    self->mMemoryUsage.resize(id.mIndex + 1);
                self->mMemoryUsage[id.mIndex] += bigAllocDelta;
            }
            if (id.mContainer)
            {
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

        protectedCall([&](LuaView& view) {
            auto& sol = view.sol();
            sol.open_libraries(sol::lib::base, sol::lib::coroutine, sol::lib::math, sol::lib::bit32, sol::lib::string,
                sol::lib::table, sol::lib::os, sol::lib::debug);

#ifndef NO_LUAJIT
            sol.open_libraries(sol::lib::jit);
#endif // NO_LUAJIT

            sol["math"]["randomseed"](static_cast<unsigned>(std::time(nullptr)));
            sol["math"]["randomseed"] = [] {};

            sol["utf8"] = LuaUtf8::initUtf8Package(sol);

            sol["writeToLog"] = [](std::string_view s) { Log(Debug::Level::Info) << s; };

            sol["setEnvironment"]
                = [](const sol::environment& env, const sol::function& fn) { sol::set_environment(env, fn); };
            sol["loadFromVFS"] = [this](std::string_view packageName) {
                return loadScriptAndCache(packageNameToVfsPath(packageName, *mVFS));
            };
            sol["loadInternalLib"] = [this](std::string_view packageName) { return loadInternalLib(packageName); };

            // Some fixes for compatibility between different Lua versions
            if (sol["unpack"] == sol::nil)
                sol["unpack"] = sol["table"]["unpack"];
            else if (sol["table"]["unpack"] == sol::nil)
                sol["table"]["unpack"] = sol["unpack"];
            if (LUA_VERSION_NUM <= 501)
            {
                sol.script(R"(
                    local _pairs = pairs
                    local _ipairs = ipairs
                    pairs = function(v) return (rawget(getmetatable(v) or {}, '__pairs') or _pairs)(v) end
                    ipairs = function(v) return (rawget(getmetatable(v) or {}, '__ipairs') or _ipairs)(v) end
                )");
            }

            sol.script(R"(
                local printToLog = function(...)
                    local strs = {}
                    for i = 1, select('#', ...) do
                        strs[i] = tostring(select(i, ...))
                    end
                    return writeToLog(table.concat(strs, '\t'))
                end
                printGen = function(name) return function(...) return printToLog(name, ...) end end

                function requireGen(env, loaded, loadFn)
                    return function(packageName)
                        local p = loaded[packageName]
                        if p == nil then
                            local loader = loadFn(packageName)
                            setEnvironment(env, loader)
                            p = loader(packageName)
                            loaded[packageName] = p
                        end
                        return p
                    end
                end

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
                function lenForReadOnly(v)
                    return #getmetatable(v).t
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

            mSandboxEnv = sol::table(sol, sol::create);
            mSandboxEnv["_VERSION"] = sol["_VERSION"];
            for (const std::string& s : safeFunctions)
            {
                if (sol[s] == sol::nil)
                    throw std::logic_error("Lua function not found: " + s);
                mSandboxEnv[s] = sol[s];
            }
            for (const std::string& s : safePackages)
            {
                if (sol[s] == sol::nil)
                    throw std::logic_error("Lua package not found: " + s);
                mCommonPackages[s] = mSandboxEnv[s] = makeReadOnly(sol[s]);
            }
            mSandboxEnv["getmetatable"] = sol["getSafeMetatable"];
            mCommonPackages["os"] = mSandboxEnv["os"]
                = makeReadOnly(tableFromPairs<std::string_view, sol::function>(sol,
                    { { "date", sol["os"]["date"] }, { "difftime", sol["os"]["difftime"] },
                        { "time", sol["os"]["time"] } }));
        });
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
            meta[sol::meta_method::index] = lua["createStrictIndexFn"](table);
        else
            meta[sol::meta_method::index] = table;
        meta[sol::meta_method::pairs] = lua["pairsForReadOnly"];
        meta[sol::meta_method::ipairs] = lua["ipairsForReadOnly"];
        meta[sol::meta_method::length] = lua["lenForReadOnly"];
        meta[sol::meta_method::type] = "ReadOnlyTable";

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
        mCommonPackages.insert_or_assign(std::move(packageName), std::move(package));
    }

    sol::protected_function_result LuaState::runInNewSandbox(const VFS::Path::Normalized& path,
        const std::string& envName, const std::map<std::string, sol::main_object>& packages,
        const sol::main_object& hiddenData)
    {
        // TODO
        sol::protected_function script = loadScriptAndCache(path);

        sol::environment env(mSol, sol::create, mSandboxEnv);
        env["print"] = mSol["printGen"](envName + ":");
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
        env["require"] = mSol["requireGen"](env, loaded, mSol["loadFromVFS"]);

        sol::set_environment(env, script);
        return call(scriptId, script);
    }

    sol::environment LuaState::newInternalLibEnvironment()
    {
        // TODO
        sol::environment env(mSol, sol::create, mSandboxEnv);
        sol::table loaded(mSol, sol::create);
        for (const std::string& s : safePackages)
            loaded[s] = static_cast<sol::object>(mSandboxEnv[s]);
        env["require"] = mSol["requireGen"](env, loaded, mSol["loadInternalLib"]);
        return env;
    }

    sol::protected_function_result LuaState::throwIfError(sol::protected_function_result&& res)
    {
        if (!res.valid())
            throw std::runtime_error(std::string("Lua error: ") += res.get<sol::error>().what());
        else
            return std::move(res);
    }

    sol::function LuaState::loadScriptAndCache(const VFS::Path::Normalized& path)
    {
        auto iter = mCompiledScripts.find(path);
        if (iter != mCompiledScripts.end())
        {
            sol::load_result res = mSol.load(iter->second.as_string_view(), path.value(), sol::load_mode::binary);
            // Unless we have memory corruption issues, the bytecode is valid at this point, but loading might still
            // fail because we've hit our Lua memory cap
            if (!res.valid())
                throw std::runtime_error("Lua error: " + res.get<std::string>());
            return res;
        }
        sol::function res = loadFromVFS(path);
        mCompiledScripts[path] = res.dump();
        return res;
    }

    sol::function LuaState::loadFromVFS(const VFS::Path::Normalized& path)
    {
        std::string fileContent(std::istreambuf_iterator<char>(*mVFS->get(path)), {});
        sol::load_result res = mSol.load(fileContent, path.value(), sol::load_mode::text);
        if (!res.valid())
            throw std::runtime_error(std::string("Lua error: ") += res.get<sol::error>().what());
        return res;
    }

    sol::function LuaState::loadInternalLib(std::string_view libName)
    {
        const auto path = packageNameToPath(libName, mLibSearchPaths);
        std::ifstream stream(path);
        std::string fileContent(std::istreambuf_iterator<char>(stream), {});
        sol::load_result res = mSol.load(fileContent, Files::pathToUnicodeString(path), sol::load_mode::text);
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

    std::string internal::formatCastingError(const sol::object& obj, const std::type_info& t)
    {
        const char* typeName = t.name();
        if (t == typeid(int))
            typeName = "int";
        else if (t == typeid(unsigned))
            typeName = "uint32";
        else if (t == typeid(size_t))
            typeName = "size_t";
        else if (t == typeid(float))
            typeName = "float";
        else if (t == typeid(double))
            typeName = "double";
        else if (t == typeid(sol::table))
            typeName = "sol::table";
        else if (t == typeid(sol::function) || t == typeid(sol::protected_function))
            typeName = "sol::function";
        else if (t == typeid(std::string) || t == typeid(std::string_view))
            typeName = "string";
        return std::string("Value \"") + toString(obj) + std::string("\" can not be casted to ") + typeName;
    }
}
