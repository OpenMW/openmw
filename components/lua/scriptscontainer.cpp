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

    std::string ScriptsContainer::ScriptId::toString() const
    {
        std::string res = mContainer->mNamePrefix;
        res.push_back('[');
        res.append(mPath);
        res.push_back(']');
        return res;
    }

    ScriptsContainer::ScriptsContainer(LuaUtil::LuaState* lua, std::string_view namePrefix, ESM::LuaScriptCfg::Flags autoStartMode)
        : mNamePrefix(namePrefix), mLua(*lua), mAutoStartMode(autoStartMode)
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
        assert(mLua.getConfiguration()[scriptId].mFlags & ESM::LuaScriptCfg::sCustom);
        std::optional<sol::function> onInit, onLoad;
        bool ok = addScript(scriptId, onInit, onLoad);
        if (ok && onInit)
            callOnInit(scriptId, *onInit);
        return ok;
    }

    void ScriptsContainer::addAutoStartedScripts()
    {
        for (int scriptId : mLua.getConfiguration().getListByFlag(mAutoStartMode))
        {
            std::optional<sol::function> onInit, onLoad;
            bool ok = addScript(scriptId, onInit, onLoad);
            if (ok && onInit)
                callOnInit(scriptId, *onInit);
        }
    }

    bool ScriptsContainer::addScript(int scriptId, std::optional<sol::function>& onInit, std::optional<sol::function>& onLoad)
    {
        assert(scriptId >= 0 && scriptId < static_cast<int>(mLua.getConfiguration().size()));
        if (mScripts.count(scriptId) != 0)
            return false;  // already present

        const std::string& path = scriptPath(scriptId);
        try
        {
            Script& script = mScripts[scriptId];
            script.mHiddenData = mLua.newTable();
            script.mHiddenData[ScriptId::KEY] = ScriptId{this, scriptId, path};
            sol::object scriptOutput = mLua.runInNewSandbox(path, mNamePrefix, mAPI, script.mHiddenData);
            if (scriptOutput == sol::nil)
                return true;
            sol::object engineHandlers = sol::nil, eventHandlers = sol::nil;
            for (const auto& [key, value] : sol::table(scriptOutput))
            {
                std::string_view sectionName = key.as<std::string_view>();
                if (sectionName == ENGINE_HANDLERS)
                    engineHandlers = value;
                else if (sectionName == EVENT_HANDLERS)
                    eventHandlers = value;
                else if (sectionName == INTERFACE_NAME)
                    script.mInterfaceName = value.as<std::string>();
                else if (sectionName == INTERFACE)
                    script.mInterface = value.as<sol::table>();
                else
                    Log(Debug::Error) << "Not supported section '" << sectionName << "' in " << mNamePrefix << "[" << path << "]";
            }
            if (engineHandlers != sol::nil)
            {
                for (const auto& [key, fn] : sol::table(engineHandlers))
                {
                    std::string_view handlerName = key.as<std::string_view>();
                    if (handlerName == HANDLER_INIT)
                        onInit = sol::function(fn);
                    else if (handlerName == HANDLER_LOAD)
                        onLoad = sol::function(fn);
                    else if (handlerName == HANDLER_SAVE)
                        script.mOnSave = sol::function(fn);
                    else if (handlerName == HANDLER_INTERFACE_OVERRIDE)
                        script.mOnOverride = sol::function(fn);
                    else
                    {
                        auto it = mEngineHandlers.find(handlerName);
                        if (it == mEngineHandlers.end())
                            Log(Debug::Error) << "Not supported handler '" << handlerName
                                              << "' in " << mNamePrefix << "[" << path << "]";
                        else
                            insertHandler(it->second->mList, scriptId, fn);
                    }
                }
            }
            if (eventHandlers != sol::nil)
            {
                for (const auto& [key, fn] : sol::table(eventHandlers))
                {
                    std::string_view eventName = key.as<std::string_view>();
                    auto it = mEventHandlers.find(eventName);
                    if (it == mEventHandlers.end())
                        it = mEventHandlers.emplace(std::string(eventName), EventHandlerList()).first;
                    insertHandler(it->second, scriptId, fn);
                }
            }

            if (script.mInterfaceName.empty() == script.mInterface.has_value())
            {
                Log(Debug::Error) << mNamePrefix << "[" << path << "]: 'interfaceName' should always be used together with 'interface'";
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
            mScripts.erase(scriptId);
            Log(Debug::Error) << "Can't start " << mNamePrefix << "[" << path << "]; " << e.what();
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
        script.mHiddenData[ScriptId::KEY] = sol::nil;
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
                if (res != sol::nil && !res.as<bool>())
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

    void ScriptsContainer::callOnInit(int scriptId, const sol::function& onInit)
    {
        try
        {
            const std::string& data = mLua.getConfiguration()[scriptId].mInitializationData;
            LuaUtil::call(onInit, deserialize(mLua.sol(), data, mSerializer));
        }
        catch (std::exception& e) { printError(scriptId, "onInit failed", e); }
    }

    void ScriptsContainer::save(ESM::LuaScripts& data)
    {
        std::map<int, std::vector<ESM::LuaTimer>> timers;
        auto saveTimerFn = [&](const Timer& timer, TimeUnit timeUnit)
        {
            if (!timer.mSerializable)
                return;
            ESM::LuaTimer savedTimer;
            savedTimer.mTime = timer.mTime;
            savedTimer.mUnit = timeUnit;
            savedTimer.mCallbackName = std::get<std::string>(timer.mCallback);
            savedTimer.mCallbackArgument = timer.mSerializedArg;
            timers[timer.mScriptId].push_back(std::move(savedTimer));
        };
        for (const Timer& timer : mSecondsTimersQueue)
            saveTimerFn(timer, TimeUnit::SECONDS);
        for (const Timer& timer : mHoursTimersQueue)
            saveTimerFn(timer, TimeUnit::HOURS);
        data.mScripts.clear();
        for (auto& [scriptId, script] : mScripts)
        {
            ESM::LuaScript savedScript;
            savedScript.mScriptPath = script.mHiddenData.get<ScriptId>(ScriptId::KEY).mPath;
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

        std::map<int, const ESM::LuaScript*> scripts;
        for (int scriptId : mLua.getConfiguration().getListByFlag(mAutoStartMode))
            scripts[scriptId] = nullptr;
        for (const ESM::LuaScript& s : data.mScripts)
        {
            std::optional<int> scriptId = cfg.findId(s.mScriptPath);
            if (!scriptId)
            {
                Log(Debug::Verbose) << "Ignoring " << mNamePrefix << "[" << s.mScriptPath << "]; script not registered";
                continue;
            }
            if (!(cfg[*scriptId].mFlags & (ESM::LuaScriptCfg::sCustom | mAutoStartMode)))
            {
                Log(Debug::Verbose) << "Ignoring " << mNamePrefix << "[" << s.mScriptPath << "]; this script is not allowed here";
                continue;
            }
            scripts[*scriptId] = &s;
        }

        for (const auto& [scriptId, savedScript] : scripts)
        {
            std::optional<sol::function> onInit, onLoad;
            if (!addScript(scriptId, onInit, onLoad))
                continue;
            if (savedScript == nullptr)
            {
                if (onInit)
                    callOnInit(scriptId, *onInit);
                continue;
            }
            if (onLoad)
            {
                try
                {
                    sol::object state = deserialize(mLua.sol(), savedScript->mData, mSerializer);
                    sol::object initializationData =
                        deserialize(mLua.sol(), mLua.getConfiguration()[scriptId].mInitializationData, mSerializer);
                    LuaUtil::call(*onLoad, state, initializationData);
                }
                catch (std::exception& e) { printError(scriptId, "onLoad failed", e); }
            }
            for (const ESM::LuaTimer& savedTimer : savedScript->mTimers)
            {
                Timer timer;
                timer.mCallback = savedTimer.mCallbackName;
                timer.mSerializable = true;
                timer.mScriptId = scriptId;
                timer.mTime = savedTimer.mTime;

                try
                {
                    timer.mArg = deserialize(mLua.sol(), savedTimer.mCallbackArgument, mSerializer);
                    // It is important if the order of content files was changed. The deserialize-serialize procedure
                    // updates refnums, so timer.mSerializedArg may be not equal to savedTimer.mCallbackArgument.
                    timer.mSerializedArg = serialize(timer.mArg, mSerializer);

                    if (savedTimer.mUnit == TimeUnit::HOURS)
                        mHoursTimersQueue.push_back(std::move(timer));
                    else
                        mSecondsTimersQueue.push_back(std::move(timer));
                }
                catch (std::exception& e) { printError(scriptId, "can not load timer", e); }
            }
        }

        std::make_heap(mSecondsTimersQueue.begin(), mSecondsTimersQueue.end());
        std::make_heap(mHoursTimersQueue.begin(), mHoursTimersQueue.end());
    }

    ScriptsContainer::~ScriptsContainer()
    {
        for (auto& [_, script] : mScripts)
            script.mHiddenData[ScriptId::KEY] = sol::nil;
    }

    // Note: shouldn't be called from destructor because mEngineHandlers has pointers on
    // external objects that are already removed during child class destruction.
    void ScriptsContainer::removeAllScripts()
    {
        for (auto& [_, script] : mScripts)
            script.mHiddenData[ScriptId::KEY] = sol::nil;
        mScripts.clear();
        for (auto& [_, handlers] : mEngineHandlers)
            handlers->mList.clear();
        mEventHandlers.clear();
        mSecondsTimersQueue.clear();
        mHoursTimersQueue.clear();

        mPublicInterfaces.clear();
        // Assigned by LuaUtil::makeReadOnly, but `clear` removes it, so we need to assign it again.
        mPublicInterfaces[sol::meta_function::index] = mPublicInterfaces;
    }

    ScriptsContainer::Script& ScriptsContainer::getScript(int scriptId)
    {
        auto it = mScripts.find(scriptId);
        if (it == mScripts.end())
            throw std::logic_error("Script doesn't exist");
        return it->second;
    }

    void ScriptsContainer::registerTimerCallback(int scriptId, std::string_view callbackName, sol::function callback)
    {
        getScript(scriptId).mRegisteredCallbacks.emplace(std::string(callbackName), std::move(callback));
    }

    void ScriptsContainer::insertTimer(std::vector<Timer>& timerQueue, Timer&& t)
    {
        timerQueue.push_back(std::move(t));
        std::push_heap(timerQueue.begin(), timerQueue.end());
    }

    void ScriptsContainer::setupSerializableTimer(TimeUnit timeUnit, double time, int scriptId,
                                                  std::string_view callbackName, sol::object callbackArg)
    {
        Timer t;
        t.mCallback = std::string(callbackName);
        t.mScriptId = scriptId;
        t.mSerializable = true;
        t.mTime = time;
        t.mArg = callbackArg;
        t.mSerializedArg = serialize(t.mArg, mSerializer);
        insertTimer(timeUnit == TimeUnit::HOURS ? mHoursTimersQueue : mSecondsTimersQueue, std::move(t));
    }

    void ScriptsContainer::setupUnsavableTimer(TimeUnit timeUnit, double time, int scriptId, sol::function callback)
    {
        Timer t;
        t.mScriptId = scriptId;
        t.mSerializable = false;
        t.mTime = time;

        t.mCallback = mTemporaryCallbackCounter;
        getScript(t.mScriptId).mTemporaryCallbacks.emplace(mTemporaryCallbackCounter, std::move(callback));
        mTemporaryCallbackCounter++;

        insertTimer(timeUnit == TimeUnit::HOURS ? mHoursTimersQueue : mSecondsTimersQueue, std::move(t));
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

    void ScriptsContainer::processTimers(double gameSeconds, double gameHours)
    {
        updateTimerQueue(mSecondsTimersQueue, gameSeconds);
        updateTimerQueue(mHoursTimersQueue, gameHours);
    }

}
