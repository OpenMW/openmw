#include "scriptscontainer.hpp"

#include <components/esm/luascripts.hpp>

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

    ScriptsContainer::ScriptsContainer(LuaUtil::LuaState* lua, std::string_view namePrefix)
        : mNamePrefix(namePrefix), mLua(*lua)
    {
        registerEngineHandlers({&mUpdateHandlers});
        mPublicInterfaces = sol::table(lua->sol(), sol::create);
        addPackage("openmw.interfaces", mPublicInterfaces);
    }

    void ScriptsContainer::printError(int scriptId, std::string_view msg, const std::exception& e)
    {
        Log(Debug::Error) << mNamePrefix << "[" << scriptPath(scriptId) << "] " << msg << ": " << e.what();
    }

    void ScriptsContainer::addPackage(std::string packageName, sol::object package)
    {
        mAPI.emplace(std::move(packageName), makeReadOnly(std::move(package)));
    }

    bool ScriptsContainer::addCustomScript(int scriptId)
    {
        const ScriptsConfiguration& conf = mLua.getConfiguration();
        assert(conf.isCustomScript(scriptId));
        std::optional<sol::function> onInit, onLoad;
        bool ok = addScript(scriptId, onInit, onLoad);
        if (ok && onInit)
            callOnInit(scriptId, *onInit, conf[scriptId].mInitializationData);
        return ok;
    }

    void ScriptsContainer::addAutoStartedScripts()
    {
        for (const auto& [scriptId, data] : mAutoStartScripts)
        {
            std::optional<sol::function> onInit, onLoad;
            bool ok = addScript(scriptId, onInit, onLoad);
            if (ok && onInit)
                callOnInit(scriptId, *onInit, data);
        }
    }

    bool ScriptsContainer::addScript(int scriptId, std::optional<sol::function>& onInit, std::optional<sol::function>& onLoad)
    {
        assert(scriptId >= 0 && scriptId < static_cast<int>(mLua.getConfiguration().size()));
        if (mScripts.count(scriptId) != 0)
            return false;  // already present

        const std::string& path = scriptPath(scriptId);
        std::string debugName = mNamePrefix;
        debugName.push_back('[');
        debugName.append(path);
        debugName.push_back(']');

        Script& script = mScripts[scriptId];
        script.mHiddenData = mLua.newTable();
        script.mHiddenData[sScriptIdKey] = ScriptId{this, scriptId};
        script.mHiddenData[sScriptDebugNameKey] = debugName;
        script.mPath = path;

        try
        {
            sol::object scriptOutput = mLua.runInNewSandbox(path, mNamePrefix, mAPI, script.mHiddenData);
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
                        onInit = fn;
                    else if (handlerName == HANDLER_LOAD)
                        onLoad = fn;
                    else if (handlerName == HANDLER_SAVE)
                        script.mOnSave = fn;
                    else if (handlerName == HANDLER_INTERFACE_OVERRIDE)
                        script.mOnOverride = fn;
                    else
                    {
                        auto it = mEngineHandlers.find(handlerName);
                        if (it == mEngineHandlers.end())
                            Log(Debug::Error) << "Not supported handler '" << handlerName << "' in " << debugName;
                        else
                            insertHandler(it->second->mList, scriptId, fn);
                    }
                }
            }
            if (eventHandlers != sol::nil)
            {
                for (const auto& [key, fn] : cast<sol::table>(eventHandlers))
                {
                    std::string_view eventName = cast<std::string_view>(key);
                    auto it = mEventHandlers.find(eventName);
                    if (it == mEventHandlers.end())
                        it = mEventHandlers.emplace(std::string(eventName), EventHandlerList()).first;
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
            mScripts[scriptId].mHiddenData[sScriptIdKey] = sol::nil;
            mScripts.erase(scriptId);
            Log(Debug::Error) << "Can't start " << debugName << "; " << e.what();
            return false;
        }
    }

    void ScriptsContainer::removeScript(int scriptId)
    {
        auto scriptIter = mScripts.find(scriptId);
        if (scriptIter == mScripts.end())
            return;  // no such script
        Script& script = scriptIter->second;
        if (script.mInterface)
            removeInterface(scriptId, script);
        script.mHiddenData[sScriptIdKey] = sol::nil;
        mScripts.erase(scriptIter);
        for (auto& [_, handlers] : mEngineHandlers)
            removeHandler(handlers->mList, scriptId);
        for (auto& [_, handlers] : mEventHandlers)
            removeHandler(handlers, scriptId);
    }

    void ScriptsContainer::insertInterface(int scriptId, const Script& script)
    {
        assert(script.mInterface);
        const Script* prev = nullptr;
        const Script* next = nullptr;
        int nextId = 0;
        for (const auto& [otherId, otherScript] : mScripts)
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
            try { LuaUtil::call(*script.mOnOverride, *prev->mInterface); }
            catch (std::exception& e) { printError(scriptId, "onInterfaceOverride failed", e); }
        }
        if (next && next->mOnOverride)
        {
            try { LuaUtil::call(*next->mOnOverride, *script.mInterface); }
            catch (std::exception& e) { printError(nextId, "onInterfaceOverride failed", e); }
        }
        if (next == nullptr)
            mPublicInterfaces[script.mInterfaceName] = *script.mInterface;
    }

    void ScriptsContainer::removeInterface(int scriptId, const Script& script)
    {
        assert(script.mInterface);
        const Script* prev = nullptr;
        const Script* next = nullptr;
        int nextId = 0;
        for (const auto& [otherId, otherScript] : mScripts)
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
                sol::object prevInterface = sol::nil;
                if (prev)
                    prevInterface = *prev->mInterface;
                try { LuaUtil::call(*next->mOnOverride, prevInterface); }
                catch (std::exception& e) { printError(nextId, "onInterfaceOverride failed", e); }
            }
        }
        else if (prev)
            mPublicInterfaces[script.mInterfaceName] = *prev->mInterface;
        else
            mPublicInterfaces[script.mInterfaceName] = sol::nil;
    }

    void ScriptsContainer::insertHandler(std::vector<Handler>& list, int scriptId, sol::function fn)
    {
        list.emplace_back();
        int pos = list.size() - 1;
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
        list.erase(std::remove_if(list.begin(), list.end(),
                                  [scriptId](const Handler& h){ return h.mScriptId == scriptId; }),
                   list.end());
    }

    void ScriptsContainer::receiveEvent(std::string_view eventName, std::string_view eventData)
    {
        auto it = mEventHandlers.find(eventName);
        if (it == mEventHandlers.end())
        {
            Log(Debug::Warning) << mNamePrefix << " has received event '" << eventName << "', but there are no handlers for this event";
            return;
        }
        sol::object data;
        try
        {
            data = LuaUtil::deserialize(mLua.sol(), eventData, mSerializer);
        }
        catch (std::exception& e)
        {
            Log(Debug::Error) << mNamePrefix << " can not parse eventData for '" << eventName << "': " << e.what();
            return;
        }
        EventHandlerList& list = it->second;
        for (int i = list.size() - 1; i >= 0; --i)
        {
            try
            {
                sol::object res = LuaUtil::call(list[i].mFn, data);
                if (res.is<bool>() && !res.as<bool>())
                    break;  // Skip other handlers if 'false' was returned.
            }
            catch (std::exception& e)
            {
                Log(Debug::Error) << mNamePrefix << "[" << scriptPath(list[i].mScriptId)
                                  << "] eventHandler[" << eventName << "] failed. " << e.what();
            }
        }
    }

    void ScriptsContainer::registerEngineHandlers(std::initializer_list<EngineHandlerList*> handlers)
    {
        for (EngineHandlerList* h : handlers)
            mEngineHandlers[h->mName] = h;
    }

    void ScriptsContainer::callOnInit(int scriptId, const sol::function& onInit, std::string_view data)
    {
        try
        {
            LuaUtil::call(onInit, deserialize(mLua.sol(), data, mSerializer));
        }
        catch (std::exception& e) { printError(scriptId, "onInit failed", e); }
    }

    void ScriptsContainer::save(ESM::LuaScripts& data)
    {
        std::map<int, std::vector<ESM::LuaTimer>> timers;
        auto saveTimerFn = [&](const Timer& timer, TimerType timerType)
        {
            if (!timer.mSerializable)
                return;
            ESM::LuaTimer savedTimer;
            savedTimer.mTime = timer.mTime;
            savedTimer.mType = timerType;
            savedTimer.mCallbackName = std::get<std::string>(timer.mCallback);
            savedTimer.mCallbackArgument = timer.mSerializedArg;
            timers[timer.mScriptId].push_back(std::move(savedTimer));
        };
        for (const Timer& timer : mSimulationTimersQueue)
            saveTimerFn(timer, TimerType::SIMULATION_TIME);
        for (const Timer& timer : mGameTimersQueue)
            saveTimerFn(timer, TimerType::GAME_TIME);
        data.mScripts.clear();
        for (auto& [scriptId, script] : mScripts)
        {
            ESM::LuaScript savedScript;
            // Note: We can not use `scriptPath(scriptId)` here because `save` can be called during
            // evaluating "reloadlua" command when ScriptsConfiguration is already changed.
            savedScript.mScriptPath = script.mPath;
            if (script.mOnSave)
            {
                try
                {
                    sol::object state = LuaUtil::call(*script.mOnSave);
                    savedScript.mData = serialize(state, mSerializer);
                }
                catch (std::exception& e) { printError(scriptId, "onSave failed", e); }
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

        struct ScriptInfo
        {
            std::string_view mInitData;
            const ESM::LuaScript* mSavedData;
        };
        std::map<int, ScriptInfo> scripts;
        for (const auto& [scriptId, initData] : mAutoStartScripts)
            scripts[scriptId] = {initData, nullptr};
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
                scripts[*scriptId] = {cfg[*scriptId].mInitializationData, &s};
            else
                Log(Debug::Verbose) << "Ignoring " << mNamePrefix << "[" << s.mScriptPath << "]; this script is not allowed here";
        }

        for (const auto& [scriptId, scriptInfo] : scripts)
        {
            std::optional<sol::function> onInit, onLoad;
            if (!addScript(scriptId, onInit, onLoad))
                continue;
            if (scriptInfo.mSavedData == nullptr)
            {
                if (onInit)
                    callOnInit(scriptId, *onInit, scriptInfo.mInitData);
                continue;
            }
            if (onLoad)
            {
                try
                {
                    sol::object state = deserialize(mLua.sol(), scriptInfo.mSavedData->mData, mSavedDataDeserializer);
                    sol::object initializationData =
                        deserialize(mLua.sol(), scriptInfo.mInitData, mSerializer);
                    LuaUtil::call(*onLoad, state, initializationData);
                }
                catch (std::exception& e) { printError(scriptId, "onLoad failed", e); }
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
                    timer.mArg = sol::main_object(
                        deserialize(mLua.sol(), savedTimer.mCallbackArgument, mSavedDataDeserializer));
                    // It is important if the order of content files was changed. The deserialize-serialize procedure
                    // updates refnums, so timer.mSerializedArg may be not equal to savedTimer.mCallbackArgument.
                    timer.mSerializedArg = serialize(timer.mArg, mSerializer);

                    if (savedTimer.mType == TimerType::GAME_TIME)
                        mGameTimersQueue.push_back(std::move(timer));
                    else
                        mSimulationTimersQueue.push_back(std::move(timer));
                }
                catch (std::exception& e) { printError(scriptId, "can not load timer", e); }
            }
        }

        std::make_heap(mSimulationTimersQueue.begin(), mSimulationTimersQueue.end());
        std::make_heap(mGameTimersQueue.begin(), mGameTimersQueue.end());
    }

    ScriptsContainer::~ScriptsContainer()
    {
        for (auto& [_, script] : mScripts)
            script.mHiddenData[sScriptIdKey] = sol::nil;
    }

    // Note: shouldn't be called from destructor because mEngineHandlers has pointers on
    // external objects that are already removed during child class destruction.
    void ScriptsContainer::removeAllScripts()
    {
        for (auto& [_, script] : mScripts)
            script.mHiddenData[sScriptIdKey] = sol::nil;
        mScripts.clear();
        for (auto& [_, handlers] : mEngineHandlers)
            handlers->mList.clear();
        mEventHandlers.clear();
        mSimulationTimersQueue.clear();
        mGameTimersQueue.clear();
        mPublicInterfaces.clear();
    }

    ScriptsContainer::Script& ScriptsContainer::getScript(int scriptId)
    {
        auto it = mScripts.find(scriptId);
        if (it == mScripts.end())
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
        t.mArg = callbackArg;
        t.mSerializedArg = serialize(t.mArg, mSerializer);
        insertTimer(type == TimerType::GAME_TIME ? mGameTimersQueue : mSimulationTimersQueue, std::move(t));
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

        insertTimer(type == TimerType::GAME_TIME ? mGameTimersQueue : mSimulationTimersQueue, std::move(t));
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
                LuaUtil::call(it->second, t.mArg);
            }
            else
            {
                int64_t id = std::get<int64_t>(t.mCallback);
                LuaUtil::call(script.mTemporaryCallbacks.at(id));
                script.mTemporaryCallbacks.erase(id);
            }
        }
        catch (std::exception& e) { printError(t.mScriptId, "callTimer failed", e); }
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
        updateTimerQueue(mSimulationTimersQueue, simulationTime);
        updateTimerQueue(mGameTimersQueue, gameTime);
    }

}
