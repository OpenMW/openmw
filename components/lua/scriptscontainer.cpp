#include "scriptscontainer.hpp"

#include "scripttracker.hpp"

#include <components/esm/luascripts.hpp>

namespace
{
    struct ScriptInfo
    {
        std::string_view mInitData;
        const ESM::LuaScript* mSavedData;
    };
}

namespace LuaUtil
{
    static constexpr std::string_view ENGINE_HANDLERS = "engineHandlers";
    static constexpr std::string_view EVENT_HANDLERS = "eventHandlers";

    static constexpr std::string_view INTERFACE_NAME = "interfaceName";
    static constexpr std::string_view INTERFACE = "interface";

    static constexpr std::string_view HANDLER_INIT = "onInit";
    static constexpr std::string_view HANDLER_SAVE = "onSave";
    static constexpr std::string_view HANDLER_LOAD = "onLoad";
    static constexpr std::string_view HANDLER_INTERFACE_OVERRIDE = "onInterfaceOverride";

    int64_t ScriptsContainer::sInstanceCount = 0;

    ScriptsContainer::ScriptsContainer(
        LuaUtil::LuaState* lua, std::string_view namePrefix, ScriptTracker* tracker, bool load)
        : mNamePrefix(namePrefix)
        , mLua(*lua)
        , mThis(std::make_shared<ScriptsContainer*>(this))
        , mTracker(tracker)
    {
        sInstanceCount++;
        registerEngineHandlers({ &mUpdateHandlers });
        if (load)
        {
            LoadedData& data = mData.emplace<LoadedData>();
            mLua.protectedCall([&](LuaView& view) {
                data.mPublicInterfaces = sol::table(view.sol(), sol::create);
                addPackage("openmw.interfaces", makeReadOnly(data.mPublicInterfaces));
            });
        }
    }

    void ScriptsContainer::printError(int scriptId, std::string_view msg, const std::exception& e) const
    {
        Log(Debug::Error) << mNamePrefix << "[" << scriptPath(scriptId) << "] " << msg << ": " << e.what();
    }

    void ScriptsContainer::addPackage(std::string packageName, sol::main_object package)
    {
        if (!package.is<sol::userdata>())
            throw std::logic_error("Expected package to be read-only: " + packageName);
        mAPI.insert_or_assign(std::move(packageName), std::move(package));
    }

    bool ScriptsContainer::addCustomScript(int scriptId, std::string_view initData)
    {
        assert(mLua.getConfiguration().isCustomScript(scriptId));
        bool ok = false;
        mLua.protectedCall([&](LuaView& view) {
            std::optional<sol::function> onInit, onLoad;
            ok = addScript(view, scriptId, onInit, onLoad);
            if (ok && onInit)
                callOnInit(view, scriptId, *onInit, initData);
        });
        return ok;
    }

    void ScriptsContainer::addAutoStartedScripts()
    {
        mLua.protectedCall([&](LuaView& view) {
            for (const auto& [scriptId, data] : mAutoStartScripts)
            {
                std::optional<sol::function> onInit, onLoad;
                bool ok = addScript(view, scriptId, onInit, onLoad);
                if (ok && onInit)
                    callOnInit(view, scriptId, *onInit, data);
            }
        });
    }

    bool ScriptsContainer::addScript(
        LuaView& view, int scriptId, std::optional<sol::function>& onInit, std::optional<sol::function>& onLoad)
    {
        assert(scriptId >= 0 && scriptId < static_cast<int>(mLua.getConfiguration().size()));
        if (hasScript(scriptId))
            return false; // already present

        LoadedData& data = ensureLoaded();
        if (data.mScripts.count(scriptId) != 0)
            return false; // bail if the script we're adding was auto started

        const VFS::Path::Normalized& path = scriptPath(scriptId);
        std::string debugName = mNamePrefix;
        debugName.push_back('[');
        debugName.append(path);
        debugName.push_back(']');

        Script& script = data.mScripts[scriptId];
        script.mHiddenData = view.newTable();
        script.mHiddenData[sScriptIdKey] = ScriptId{ this, scriptId };
        script.mHiddenData[sScriptDebugNameKey] = debugName;
        script.mPath = path;
        script.mStats.mAvgInstructionCount = 0;

        const auto oldMemoryUsageIt = mRemovedScriptsMemoryUsage.find(scriptId);
        if (oldMemoryUsageIt != mRemovedScriptsMemoryUsage.end())
        {
            script.mStats.mMemoryUsage = oldMemoryUsageIt->second;
            mRemovedScriptsMemoryUsage.erase(oldMemoryUsageIt);
        }
        else
            script.mStats.mMemoryUsage = 0;

        try
        {
            sol::object scriptOutput = mLua.runInNewSandbox(path, debugName, mAPI, script.mHiddenData);
            if (scriptOutput == sol::nil)
                return true;
            sol::object engineHandlers = sol::nil, eventHandlers = sol::nil;
            for (const auto& [key, value] : cast<sol::table>(scriptOutput))
            {
                std::string_view sectionName = cast<std::string_view>(key);
                if (sectionName == ENGINE_HANDLERS)
                    engineHandlers = value;
                else if (sectionName == EVENT_HANDLERS)
                    eventHandlers = value;
                else if (sectionName == INTERFACE_NAME)
                    script.mInterfaceName = cast<std::string>(value);
                else if (sectionName == INTERFACE)
                    script.mInterface = cast<sol::table>(value);
                else
                    Log(Debug::Error) << "Not supported section '" << sectionName << "' in " << debugName;
            }
            if (engineHandlers != sol::nil)
            {
                for (const auto& [key, handler] : cast<sol::table>(engineHandlers))
                {
                    std::string_view handlerName = cast<std::string_view>(key);
                    sol::function fn = cast<sol::function>(handler);
                    if (handlerName == HANDLER_INIT)
                        onInit = std::move(fn);
                    else if (handlerName == HANDLER_LOAD)
                        onLoad = std::move(fn);
                    else if (handlerName == HANDLER_SAVE)
                        script.mOnSave = std::move(fn);
                    else if (handlerName == HANDLER_INTERFACE_OVERRIDE)
                        script.mOnOverride = std::move(fn);
                    else
                    {
                        auto it = mEngineHandlers.find(handlerName);
                        if (it == mEngineHandlers.end())
                            Log(Debug::Error) << "Not supported handler '" << handlerName << "' in " << debugName;
                        else
                            insertHandler(it->second->mList, scriptId, std::move(fn));
                    }
                }
            }
            if (eventHandlers != sol::nil)
            {
                for (const auto& [key, fn] : cast<sol::table>(eventHandlers))
                {
                    std::string_view eventName = cast<std::string_view>(key);
                    auto it = data.mEventHandlers.find(eventName);
                    if (it == data.mEventHandlers.end())
                        it = data.mEventHandlers.emplace(std::string(eventName), EventHandlerList()).first;
                    insertHandler(it->second, scriptId, cast<sol::function>(fn));
                }
            }

            if (script.mInterfaceName.empty() == script.mInterface.has_value())
            {
                Log(Debug::Error) << debugName << ": 'interfaceName' should always be used together with 'interface'";
                script.mInterfaceName.clear();
                script.mInterface = sol::nil;
            }
            else if (script.mInterface)
            {
                script.mInterface = makeReadOnly(*script.mInterface);
                insertInterface(scriptId, script);
            }

            return true;
        }
        catch (std::exception& e)
        {
            auto iter = data.mScripts.find(scriptId);
            mRemovedScriptsMemoryUsage[scriptId] = iter->second.mStats.mMemoryUsage;
            data.mScripts.erase(iter);
            Log(Debug::Error) << "Can't start " << debugName << "; " << e.what();
            return false;
        }
    }

    bool ScriptsContainer::hasScript(int scriptId) const
    {
        return std::visit(
            [&](auto&& variant) {
                using T = std::decay_t<decltype(variant)>;
                if constexpr (std::is_same_v<T, UnloadedData>)
                {
                    const auto& conf = mLua.getConfiguration();
                    if (scriptId >= 0 && static_cast<size_t>(scriptId) < conf.size())
                    {
                        const auto& path = conf[scriptId].mScriptPath;
                        for (const ESM::LuaScript& script : variant.mScripts)
                        {
                            if (script.mScriptPath == path)
                                return true;
                        }
                    }
                    return false;
                }
                else
                {
                    static_assert(std::is_same_v<T, LoadedData>, "Non-exhaustive visitor");
                    return variant.mScripts.count(scriptId) != 0;
                }
            },
            mData);
    }

    void ScriptsContainer::removeScript(int scriptId)
    {
        LoadedData& data = ensureLoaded();
        auto scriptIter = data.mScripts.find(scriptId);
        if (scriptIter == data.mScripts.end())
            return; // no such script
        Script& script = scriptIter->second;
        if (script.mInterface)
            removeInterface(scriptId, script);
        mRemovedScriptsMemoryUsage[scriptId] = script.mStats.mMemoryUsage;
        data.mScripts.erase(scriptIter);
        for (auto& [_, handlers] : mEngineHandlers)
            removeHandler(handlers->mList, scriptId);
        for (auto& [_, handlers] : data.mEventHandlers)
            removeHandler(handlers, scriptId);
    }

    void ScriptsContainer::insertInterface(int scriptId, const Script& script)
    {
        assert(script.mInterface);
        const Script* prev = nullptr;
        const Script* next = nullptr;
        int nextId = 0;
        LoadedData& data = ensureLoaded();
        for (const auto& [otherId, otherScript] : data.mScripts)
        {
            if (scriptId == otherId || script.mInterfaceName != otherScript.mInterfaceName)
                continue;
            if (otherId < scriptId)
                prev = &otherScript;
            else
            {
                next = &otherScript;
                nextId = otherId;
                break;
            }
        }
        if (prev && script.mOnOverride)
        {
            try
            {
                LuaUtil::call({ this, scriptId }, *script.mOnOverride, *prev->mInterface);
            }
            catch (std::exception& e)
            {
                printError(scriptId, "onInterfaceOverride failed", e);
            }
        }
        if (next && next->mOnOverride)
        {
            try
            {
                LuaUtil::call({ this, nextId }, *next->mOnOverride, *script.mInterface);
            }
            catch (std::exception& e)
            {
                printError(nextId, "onInterfaceOverride failed", e);
            }
        }
        if (next == nullptr)
            data.mPublicInterfaces[script.mInterfaceName] = *script.mInterface;
    }

    void ScriptsContainer::removeInterface(int scriptId, const Script& script)
    {
        assert(script.mInterface);
        const Script* prev = nullptr;
        const Script* next = nullptr;
        int nextId = 0;
        LoadedData& data = ensureLoaded();
        for (const auto& [otherId, otherScript] : data.mScripts)
        {
            if (scriptId == otherId || script.mInterfaceName != otherScript.mInterfaceName)
                continue;
            if (otherId < scriptId)
                prev = &otherScript;
            else
            {
                next = &otherScript;
                nextId = otherId;
                break;
            }
        }
        if (next)
        {
            if (next->mOnOverride)
            {
                sol::main_object prevInterface = sol::nil;
                if (prev)
                    prevInterface = *prev->mInterface;
                try
                {
                    LuaUtil::call({ this, nextId }, *next->mOnOverride, prevInterface);
                }
                catch (std::exception& e)
                {
                    printError(nextId, "onInterfaceOverride failed", e);
                }
            }
        }
        else if (prev)
            data.mPublicInterfaces[script.mInterfaceName] = *prev->mInterface;
        else
            data.mPublicInterfaces[script.mInterfaceName] = sol::nil;
    }

    void ScriptsContainer::insertHandler(std::vector<Handler>& list, int scriptId, sol::function fn)
    {
        size_t pos = list.size();
        list.emplace_back();
        while (pos > 0 && list[pos - 1].mScriptId > scriptId)
        {
            list[pos] = std::move(list[pos - 1]);
            pos--;
        }
        list[pos].mScriptId = scriptId;
        list[pos].mFn = std::move(fn);
    }

    void ScriptsContainer::removeHandler(std::vector<Handler>& list, int scriptId)
    {
        list.erase(
            std::remove_if(list.begin(), list.end(), [scriptId](const Handler& h) { return h.mScriptId == scriptId; }),
            list.end());
    }

    void ScriptsContainer::receiveEvent(std::string_view eventName, std::string_view eventData)
    {
        LoadedData& data = ensureLoaded();
        auto it = data.mEventHandlers.find(eventName);
        if (it == data.mEventHandlers.end())
            return;
        mLua.protectedCall([&](LuaView& view) {
            sol::object object;
            try
            {
                object = LuaUtil::deserialize(view.sol(), eventData, mSerializer);
            }
            catch (std::exception& e)
            {
                Log(Debug::Error) << mNamePrefix << " can not parse eventData for '" << eventName << "': " << e.what();
                return;
            }
            EventHandlerList& list = it->second;
            for (size_t i = list.size(); i > 0; --i)
            {
                const Handler& h = list[i - 1];
                try
                {
                    sol::object res = LuaUtil::call({ this, h.mScriptId }, h.mFn, object);
                    if (res.is<bool>() && !res.as<bool>())
                        break; // Skip other handlers if 'false' was returned.
                }
                catch (std::exception& e)
                {
                    Log(Debug::Error) << mNamePrefix << "[" << scriptPath(h.mScriptId) << "] eventHandler[" << eventName
                                      << "] failed. " << e.what();
                }
            }
        });
    }

    void ScriptsContainer::registerEngineHandlers(std::initializer_list<EngineHandlerList*> handlers)
    {
        for (EngineHandlerList* h : handlers)
            mEngineHandlers[h->mName] = h;
    }

    void ScriptsContainer::callOnInit(LuaView& view, int scriptId, const sol::function& onInit, std::string_view data)
    {
        try
        {
            LuaUtil::call({ this, scriptId }, onInit, deserialize(view.sol(), data, mSerializer));
        }
        catch (std::exception& e)
        {
            printError(scriptId, "onInit failed", e);
        }
    }

    void ScriptsContainer::save(ESM::LuaScripts& data)
    {
        if (const UnloadedData* unloadedData = std::get_if<UnloadedData>(&mData))
        {
            data.mScripts = unloadedData->mScripts;
            return;
        }
        mLua.protectedCall([&](LuaView& view) { save(view, data); });
    }

    void ScriptsContainer::save(LuaView&, ESM::LuaScripts& data)
    {
        if (const UnloadedData* unloadedData = std::get_if<UnloadedData>(&mData))
        {
            data.mScripts = unloadedData->mScripts;
            return;
        }
        const auto& loadedData = std::get<LoadedData>(mData);
        std::map<int, std::vector<ESM::LuaTimer>> timers;
        auto saveTimerFn = [&](const Timer& timer, TimerType timerType) {
            if (!timer.mSerializable)
                return;
            ESM::LuaTimer savedTimer;
            savedTimer.mTime = timer.mTime;
            savedTimer.mType = timerType;
            savedTimer.mCallbackName = std::get<std::string>(timer.mCallback);
            savedTimer.mCallbackArgument = timer.mSerializedArg;
            timers[timer.mScriptId].push_back(std::move(savedTimer));
        };
        for (const Timer& timer : loadedData.mSimulationTimersQueue)
            saveTimerFn(timer, TimerType::SIMULATION_TIME);
        for (const Timer& timer : loadedData.mGameTimersQueue)
            saveTimerFn(timer, TimerType::GAME_TIME);
        data.mScripts.clear();
        for (auto& [scriptId, script] : loadedData.mScripts)
        {
            ESM::LuaScript savedScript;
            // Note: We can not use `scriptPath(scriptId)` here because `save` can be called during
            // evaluating "reloadlua" command when ScriptsConfiguration is already changed.
            savedScript.mScriptPath = script.mPath;
            if (script.mOnSave)
            {
                try
                {
                    sol::object state = LuaUtil::call({ this, scriptId }, *script.mOnSave);
                    savedScript.mData = serialize(state, mSerializer);
                }
                catch (std::exception& e)
                {
                    printError(scriptId, "onSave failed", e);
                }
            }
            auto timersIt = timers.find(scriptId);
            if (timersIt != timers.end())
                savedScript.mTimers = std::move(timersIt->second);
            data.mScripts.push_back(std::move(savedScript));
        }
    }

    void ScriptsContainer::load(const ESM::LuaScripts& data)
    {
        removeAllScripts();
        const ScriptsConfiguration& cfg = mLua.getConfiguration();

        std::map<int, ScriptInfo> scripts;
        for (const auto& [scriptId, initData] : mAutoStartScripts)
            scripts[scriptId] = { initData, nullptr };
        for (const ESM::LuaScript& s : data.mScripts)
        {
            std::optional<int> scriptId = cfg.findId(s.mScriptPath);
            if (!scriptId)
            {
                Log(Debug::Verbose) << "Ignoring " << mNamePrefix << "[" << s.mScriptPath << "]; script not registered";
                continue;
            }
            auto it = scripts.find(*scriptId);
            if (it != scripts.end())
                it->second.mSavedData = &s;
            else if (cfg.isCustomScript(*scriptId))
                scripts[*scriptId] = { cfg[*scriptId].mInitializationData, &s };
            else
                Log(Debug::Verbose) << "Ignoring " << mNamePrefix << "[" << s.mScriptPath
                                    << "]; this script is not allowed here";
        }

        mLua.protectedCall([&](LuaView& view) {
            UnloadedData& container = ensureUnloaded(view);

            for (const auto& [scriptId, scriptInfo] : scripts)
            {
                if (scriptInfo.mSavedData == nullptr)
                    continue;
                ESM::LuaScript& script = container.mScripts.emplace_back(*scriptInfo.mSavedData);
                if (!script.mData.empty())
                {
                    try
                    {
                        sol::object state = deserialize(view.sol(), script.mData, mSavedDataDeserializer);
                        script.mData = serialize(state, mSerializer);
                    }
                    catch (std::exception& e)
                    {
                        printError(scriptId, "onLoad failed", e);
                        script.mData.clear();
                    }
                }
                for (auto it = script.mTimers.begin(); it != script.mTimers.end();)
                {
                    try
                    {
                        sol::object arg = deserialize(view.sol(), it->mCallbackArgument, mSavedDataDeserializer);
                        // It is important if the order of content files was changed. The deserialize-serialize
                        // procedure updates refnums, so timer.mSerializedArg may be not equal to
                        // savedTimer.mCallbackArgument.
                        it->mCallbackArgument = serialize(arg, mSerializer);
                        ++it;
                    }
                    catch (std::exception& e)
                    {
                        printError(scriptId, "can not load timer", e);
                        it = script.mTimers.erase(it);
                    }
                }
            }
        });
    }

    ScriptsContainer::LoadedData& ScriptsContainer::ensureLoaded()
    {
        mRequiredLoading = true;
        if (LoadedData* data = std::get_if<LoadedData>(&mData))
            return *data;
        UnloadedData& unloadedData = std::get<UnloadedData>(mData);
        std::vector<ESM::LuaScript> savedScripts = std::move(unloadedData.mScripts);
        LoadedData& data = mData.emplace<LoadedData>();

        const ScriptsConfiguration& cfg = mLua.getConfiguration();

        std::map<int, ScriptInfo> scripts;
        for (const auto& [scriptId, initData] : mAutoStartScripts)
            scripts[scriptId] = { initData, nullptr };
        for (const ESM::LuaScript& s : savedScripts)
        {
            std::optional<int> scriptId = cfg.findId(s.mScriptPath);
            auto it = scripts.find(*scriptId);
            if (it != scripts.end())
                it->second.mSavedData = &s;
            else if (cfg.isCustomScript(*scriptId))
                scripts[*scriptId] = { cfg[*scriptId].mInitializationData, &s };
        }

        mLua.protectedCall([&](LuaView& view) {
            data.mPublicInterfaces = sol::table(view.sol(), sol::create);
            addPackage("openmw.interfaces", makeReadOnly(data.mPublicInterfaces));

            for (const auto& [scriptId, scriptInfo] : scripts)
            {
                std::optional<sol::function> onInit, onLoad;
                if (!addScript(view, scriptId, onInit, onLoad))
                    continue;
                if (scriptInfo.mSavedData == nullptr)
                {
                    if (onInit)
                        callOnInit(view, scriptId, *onInit, scriptInfo.mInitData);
                    continue;
                }
                if (onLoad)
                {
                    try
                    {
                        sol::object state = deserialize(view.sol(), scriptInfo.mSavedData->mData, mSerializer);
                        sol::object initializationData = deserialize(view.sol(), scriptInfo.mInitData, mSerializer);
                        LuaUtil::call({ this, scriptId }, *onLoad, state, initializationData);
                    }
                    catch (std::exception& e)
                    {
                        printError(scriptId, "onLoad failed", e);
                    }
                }
                for (const ESM::LuaTimer& savedTimer : scriptInfo.mSavedData->mTimers)
                {
                    Timer timer;
                    timer.mCallback = savedTimer.mCallbackName;
                    timer.mSerializable = true;
                    timer.mScriptId = scriptId;
                    timer.mTime = savedTimer.mTime;

                    try
                    {
                        timer.mArg
                            = sol::main_object(deserialize(view.sol(), savedTimer.mCallbackArgument, mSerializer));
                        // It is important if the order of content files was changed. The deserialize-serialize
                        // procedure updates refnums, so timer.mSerializedArg may be not equal to
                        // savedTimer.mCallbackArgument.
                        timer.mSerializedArg = serialize(timer.mArg, mSerializer);

                        if (savedTimer.mType == TimerType::GAME_TIME)
                            data.mGameTimersQueue.push_back(std::move(timer));
                        else
                            data.mSimulationTimersQueue.push_back(std::move(timer));
                    }
                    catch (std::exception& e)
                    {
                        printError(scriptId, "can not load timer", e);
                    }
                }
            }
        });

        std::make_heap(data.mSimulationTimersQueue.begin(), data.mSimulationTimersQueue.end());
        std::make_heap(data.mGameTimersQueue.begin(), data.mGameTimersQueue.end());

        if (mTracker)
            mTracker->onLoad(*this);

        return data;
    }

    ScriptsContainer::UnloadedData& ScriptsContainer::ensureUnloaded(LuaView& view)
    {
        if (UnloadedData* data = std::get_if<UnloadedData>(&mData))
            return *data;
        UnloadedData data;
        save(view, data);
        mAPI.erase("openmw.interfaces");
        UnloadedData& out = mData.emplace<UnloadedData>(std::move(data));
        for (auto& [_, handlers] : mEngineHandlers)
            handlers->mList.clear();
        mRequiredLoading = false;
        return out;
    }

    ScriptsContainer::~ScriptsContainer()
    {
        sInstanceCount--;
        *mThis = nullptr;
    }

    // Note: shouldn't be called from destructor because mEngineHandlers has pointers on
    // external objects that are already removed during child class destruction.
    void ScriptsContainer::removeAllScripts()
    {
        std::visit(
            [&](auto&& variant) {
                using T = std::decay_t<decltype(variant)>;
                if constexpr (std::is_same_v<T, UnloadedData>)
                {
                    variant.mScripts.clear();
                }
                else if constexpr (std::is_same_v<T, LoadedData>)
                {
                    for (auto& [id, script] : variant.mScripts)
                    {
                        mRemovedScriptsMemoryUsage[id] = script.mStats.mMemoryUsage;
                    }
                    variant.mScripts.clear();
                    for (auto& [_, handlers] : mEngineHandlers)
                        handlers->mList.clear();
                    variant.mEventHandlers.clear();
                    variant.mSimulationTimersQueue.clear();
                    variant.mGameTimersQueue.clear();
                    variant.mPublicInterfaces.clear();
                }
            },
            mData);
    }

    ScriptsContainer::Script& ScriptsContainer::getScript(int scriptId)
    {
        LoadedData& data = ensureLoaded();
        auto it = data.mScripts.find(scriptId);
        if (it == data.mScripts.end())
            throw std::logic_error("Script doesn't exist");
        return it->second;
    }

    void ScriptsContainer::registerTimerCallback(
        int scriptId, std::string_view callbackName, sol::main_protected_function callback)
    {
        getScript(scriptId).mRegisteredCallbacks.emplace(std::string(callbackName), std::move(callback));
    }

    void ScriptsContainer::insertTimer(std::vector<Timer>& timerQueue, Timer&& t)
    {
        timerQueue.push_back(std::move(t));
        std::push_heap(timerQueue.begin(), timerQueue.end());
    }

    void ScriptsContainer::setupSerializableTimer(
        TimerType type, double time, int scriptId, std::string_view callbackName, sol::main_object callbackArg)
    {
        Timer t;
        t.mCallback = std::string(callbackName);
        t.mScriptId = scriptId;
        t.mSerializable = true;
        t.mTime = time;
        t.mArg = std::move(callbackArg);
        t.mSerializedArg = serialize(t.mArg, mSerializer);
        LoadedData& data = ensureLoaded();
        insertTimer(type == TimerType::GAME_TIME ? data.mGameTimersQueue : data.mSimulationTimersQueue, std::move(t));
    }

    void ScriptsContainer::setupUnsavableTimer(
        TimerType type, double time, int scriptId, sol::main_protected_function callback)
    {
        Timer t;
        t.mScriptId = scriptId;
        t.mSerializable = false;
        t.mTime = time;

        t.mCallback = mTemporaryCallbackCounter;
        getScript(t.mScriptId).mTemporaryCallbacks.emplace(mTemporaryCallbackCounter, std::move(callback));
        mTemporaryCallbackCounter++;
        LoadedData& data = ensureLoaded();
        insertTimer(type == TimerType::GAME_TIME ? data.mGameTimersQueue : data.mSimulationTimersQueue, std::move(t));
    }

    void ScriptsContainer::callTimer(const Timer& t)
    {
        try
        {
            Script& script = getScript(t.mScriptId);
            if (t.mSerializable)
            {
                const std::string& callbackName = std::get<std::string>(t.mCallback);
                auto it = script.mRegisteredCallbacks.find(callbackName);
                if (it == script.mRegisteredCallbacks.end())
                    throw std::logic_error("Callback '" + callbackName + "' doesn't exist");
                LuaUtil::call({ this, t.mScriptId }, it->second, t.mArg);
            }
            else
            {
                int64_t id = std::get<int64_t>(t.mCallback);
                LuaUtil::call({ this, t.mScriptId }, script.mTemporaryCallbacks.at(id));
                script.mTemporaryCallbacks.erase(id);
            }
        }
        catch (std::exception& e)
        {
            printError(t.mScriptId, "callTimer failed", e);
        }
    }

    void ScriptsContainer::updateTimerQueue(std::vector<Timer>& timerQueue, double time)
    {
        while (!timerQueue.empty() && timerQueue.front().mTime <= time)
        {
            callTimer(timerQueue.front());
            std::pop_heap(timerQueue.begin(), timerQueue.end());
            timerQueue.pop_back();
        }
    }

    void ScriptsContainer::processTimers(double simulationTime, double gameTime)
    {
        mLua.protectedCall([&](LuaView& view) {
            LoadedData& data = ensureLoaded();
            updateTimerQueue(data.mSimulationTimersQueue, simulationTime);
            updateTimerQueue(data.mGameTimersQueue, gameTime);
        });
    }

    static constexpr float instructionCountAvgCoef = 1.0f / 30; // averaging over approximately 30 frames

    void ScriptsContainer::statsNextFrame()
    {
        if (LoadedData* data = std::get_if<LoadedData>(&mData))
        {
            for (auto& [scriptId, script] : data->mScripts)
            {
                // The averaging formula is: averageValue = averageValue * (1-c) + newValue * c
                script.mStats.mAvgInstructionCount *= 1 - instructionCountAvgCoef;
                if (script.mStats.mAvgInstructionCount < 5)
                    script.mStats.mAvgInstructionCount = 0; // speeding up converge to zero if newValue is zero
            }
        }
    }

    void ScriptsContainer::addInstructionCount(int scriptId, int64_t instructionCount)
    {
        if (LoadedData* data = std::get_if<LoadedData>(&mData))
        {
            auto it = data->mScripts.find(scriptId);
            if (it != data->mScripts.end())
                it->second.mStats.mAvgInstructionCount += instructionCount * instructionCountAvgCoef;
        }
    }

    void ScriptsContainer::addMemoryUsage(int scriptId, int64_t memoryDelta)
    {
        int64_t* usage = std::visit(
            [&](auto&& variant) {
                using T = std::decay_t<decltype(variant)>;
                if constexpr (std::is_same_v<T, LoadedData>)
                {
                    auto it = variant.mScripts.find(scriptId);
                    if (it != variant.mScripts.end())
                        return &it->second.mStats.mMemoryUsage;
                }
                auto [rIt, _] = mRemovedScriptsMemoryUsage.emplace(scriptId, 0);
                return &rIt->second;
            },
            mData);
        *usage += memoryDelta;

        if (mLua.getSettings().mLogMemoryUsage)
        {
            int64_t after = *usage;
            int64_t before = after - memoryDelta;
            // Logging only if one of the most significant bits of used memory size was changed.
            // Otherwise it is too verbose.
            if ((before ^ after) * 8 > after)
                Log(Debug::Verbose) << mNamePrefix << "[" << scriptPath(scriptId) << "] memory usage " << before
                                    << " -> " << after;
        }
    }

    void ScriptsContainer::collectStats(std::vector<ScriptStats>& stats) const
    {
        stats.resize(mLua.getConfiguration().size());
        if (const LoadedData* data = std::get_if<LoadedData>(&mData))
        {
            for (auto& [id, script] : data->mScripts)
            {
                stats[id].mAvgInstructionCount += script.mStats.mAvgInstructionCount;
                stats[id].mMemoryUsage += script.mStats.mMemoryUsage;
            }
        }
        for (auto& [id, mem] : mRemovedScriptsMemoryUsage)
            stats[id].mMemoryUsage += mem;
    }

    ScriptsContainer::Script::~Script()
    {
        if (mHiddenData != sol::nil)
            mHiddenData[sScriptIdKey] = sol::nil;
    }
}
