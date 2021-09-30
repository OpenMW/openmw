#include "scriptscontainer.hpp"

#include <components/esm/luascripts.hpp>

namespace LuaUtil
{
    static constexpr std::string_view ENGINE_HANDLERS = "engineHandlers";
    static constexpr std::string_view EVENT_HANDLERS = "eventHandlers";

    static constexpr std::string_view INTERFACE_NAME = "interfaceName";
    static constexpr std::string_view INTERFACE = "interface";

    static constexpr std::string_view HANDLER_SAVE = "onSave";
    static constexpr std::string_view HANDLER_LOAD = "onLoad";

    static constexpr std::string_view REGISTERED_TIMER_CALLBACKS = "_timers";
    static constexpr std::string_view TEMPORARY_TIMER_CALLBACKS = "_temp_timers";

    std::string ScriptsContainer::ScriptId::toString() const
    {
        std::string res = mContainer->mNamePrefix;
        res.push_back('[');
        res.append(mPath);
        res.push_back(']');
        return res;
    }

    ScriptsContainer::ScriptsContainer(LuaUtil::LuaState* lua, std::string_view namePrefix) : mNamePrefix(namePrefix), mLua(*lua)
    {
        registerEngineHandlers({&mUpdateHandlers});
        mPublicInterfaces = sol::table(lua->sol(), sol::create);
        addPackage("openmw.interfaces", mPublicInterfaces);
    }

    void ScriptsContainer::addPackage(const std::string& packageName, sol::object package)
    {
        API[packageName] = makeReadOnly(std::move(package));
    }

    bool ScriptsContainer::addNewScript(const std::string& path)
    {
        if (mScripts.count(path) != 0)
            return false;  // already present

        try
        {
            sol::table hiddenData(mLua.sol(), sol::create);
            hiddenData[ScriptId::KEY] = ScriptId{this, path};
            hiddenData[REGISTERED_TIMER_CALLBACKS] = mLua.newTable();
            hiddenData[TEMPORARY_TIMER_CALLBACKS] = mLua.newTable();
            mScripts[path].mHiddenData = hiddenData;
            sol::object script = mLua.runInNewSandbox(path, mNamePrefix, API, hiddenData);
            std::string interfaceName = "";
            sol::object publicInterface = sol::nil;
            if (script != sol::nil)
            {
                for (auto& [key, value] : sol::table(script))
                {
                    std::string_view sectionName = key.as<std::string_view>();
                    if (sectionName == ENGINE_HANDLERS)
                        parseEngineHandlers(value, path);
                    else if (sectionName == EVENT_HANDLERS)
                        parseEventHandlers(value, path);
                    else if (sectionName == INTERFACE_NAME)
                        interfaceName = value.as<std::string>();
                    else if (sectionName == INTERFACE)
                        publicInterface = value.as<sol::table>();
                    else
                        Log(Debug::Error) << "Not supported section '" << sectionName << "' in " << mNamePrefix << "[" << path << "]";
                }
            }
            if (interfaceName.empty() != (publicInterface == sol::nil))
                Log(Debug::Error) << mNamePrefix << "[" << path << "]: 'interfaceName' should always be used together with 'interface'";
            else if (!interfaceName.empty())
                script.as<sol::table>()[INTERFACE] = mPublicInterfaces[interfaceName] = makeReadOnly(publicInterface);
            mScriptOrder.push_back(path);
            mScripts[path].mInterface = std::move(script);
            return true;
        }
        catch (std::exception& e)
        {
            mScripts.erase(path);
            Log(Debug::Error) << "Can't start " << mNamePrefix << "[" << path << "]; " << e.what();
            return false;
        }
    }

    bool ScriptsContainer::removeScript(const std::string& path)
    {
        auto scriptIter = mScripts.find(path);
        if (scriptIter == mScripts.end())
            return false;  // no such script
        scriptIter->second.mHiddenData[ScriptId::KEY] = sol::nil;
        sol::object& script = scriptIter->second.mInterface;
        if (getFieldOrNil(script, INTERFACE_NAME) != sol::nil)
        {
            std::string_view interfaceName = getFieldOrNil(script, INTERFACE_NAME).as<std::string_view>();
            if (mPublicInterfaces[interfaceName] == getFieldOrNil(script, INTERFACE))
            {
                mPublicInterfaces[interfaceName] = sol::nil;
                auto prevIt = mScriptOrder.rbegin();
                while (*prevIt != path)
                    prevIt++;
                prevIt++;
                while (prevIt != mScriptOrder.rend())
                {
                    sol::object& prevScript = mScripts[*(prevIt++)].mInterface;
                    sol::object prevInterfaceName = getFieldOrNil(prevScript, INTERFACE_NAME);
                    if (prevInterfaceName != sol::nil && prevInterfaceName.as<std::string_view>() == interfaceName)
                    {
                        mPublicInterfaces[interfaceName] = getFieldOrNil(prevScript, INTERFACE);
                        break;
                    }
                }
            }
        }
        sol::object engineHandlers = getFieldOrNil(script, ENGINE_HANDLERS);
        if (engineHandlers != sol::nil)
        {
            for (auto& [key, value] : sol::table(engineHandlers))
            {
                std::string_view handlerName = key.as<std::string_view>();
                auto handlerIter = mEngineHandlers.find(handlerName);
                if (handlerIter == mEngineHandlers.end())
                    continue;
                std::vector<sol::protected_function>& list = handlerIter->second->mList;
                list.erase(std::find(list.begin(), list.end(), value.as<sol::protected_function>()));
            }
        }
        sol::object eventHandlers = getFieldOrNil(script, EVENT_HANDLERS);
        if (eventHandlers != sol::nil)
        {
            for (auto& [key, value] : sol::table(eventHandlers))
            {
                EventHandlerList& list = mEventHandlers.find(key.as<std::string_view>())->second;
                list.erase(std::find(list.begin(), list.end(), value.as<sol::protected_function>()));
            }
        }
        mScripts.erase(scriptIter);
        mScriptOrder.erase(std::find(mScriptOrder.begin(), mScriptOrder.end(), path));
        return true;
    }

    void ScriptsContainer::parseEventHandlers(sol::table handlers, std::string_view scriptPath)
    {
        for (auto& [key, value] : handlers)
        {
            std::string_view eventName = key.as<std::string_view>();
            auto it = mEventHandlers.find(eventName);
            if (it == mEventHandlers.end())
                it = mEventHandlers.insert({std::string(eventName), EventHandlerList()}).first;
            it->second.push_back(value);
        }
    }

    void ScriptsContainer::parseEngineHandlers(sol::table handlers, std::string_view scriptPath)
    {
        for (auto& [key, value] : handlers)
        {
            std::string_view handlerName = key.as<std::string_view>();
            if (handlerName == HANDLER_LOAD || handlerName == HANDLER_SAVE)
                continue;  // save and load are handled separately
            auto it = mEngineHandlers.find(handlerName);
            if (it == mEngineHandlers.end())
                Log(Debug::Error) << "Not supported handler '" << handlerName << "' in " << mNamePrefix << "[" << scriptPath << "]";
            else
                it->second->mList.push_back(value);
        }
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
                sol::object res = LuaUtil::call(list[i], data);
                if (res != sol::nil && !res.as<bool>())
                    break;  // Skip other handlers if 'false' was returned.
            }
            catch (std::exception& e)
            {
                Log(Debug::Error) << mNamePrefix << " eventHandler[" << eventName << "] failed. " << e.what();
            }
        }
    }

    void ScriptsContainer::registerEngineHandlers(std::initializer_list<EngineHandlerList*> handlers)
    {
        for (EngineHandlerList* h : handlers)
            mEngineHandlers[h->mName] = h;
    }

    void ScriptsContainer::save(ESM::LuaScripts& data)
    {
        std::map<std::string, std::vector<ESM::LuaTimer>> timers;
        auto saveTimerFn = [&](const Timer& timer, TimeUnit timeUnit)
        {
            if (!timer.mSerializable)
                return;
            ESM::LuaTimer savedTimer;
            savedTimer.mTime = timer.mTime;
            savedTimer.mUnit = timeUnit;
            savedTimer.mCallbackName = std::get<std::string>(timer.mCallback);
            savedTimer.mCallbackArgument = timer.mSerializedArg;
            if (timers.count(timer.mScript) == 0)
                timers[timer.mScript] = {};
            timers[timer.mScript].push_back(std::move(savedTimer));
        };
        for (const Timer& timer : mSecondsTimersQueue)
            saveTimerFn(timer, TimeUnit::SECONDS);
        for (const Timer& timer : mHoursTimersQueue)
            saveTimerFn(timer, TimeUnit::HOURS);
        data.mScripts.clear();
        for (const std::string& path : mScriptOrder)
        {
            ESM::LuaScript savedScript;
            savedScript.mScriptPath = path;
            sol::object handler = getFieldOrNil(mScripts[path].mInterface, ENGINE_HANDLERS, HANDLER_SAVE);
            if (handler != sol::nil)
            {
                try
                {
                    sol::object state = LuaUtil::call(handler);
                    savedScript.mData = serialize(state, mSerializer);
                }
                catch (std::exception& e)
                {
                    Log(Debug::Error) << mNamePrefix << "[" << path << "] onSave failed: " << e.what();
                }
            }
            auto timersIt = timers.find(path);
            if (timersIt != timers.end())
                savedScript.mTimers = std::move(timersIt->second);
            data.mScripts.push_back(std::move(savedScript));
        }
    }

    void ScriptsContainer::load(const ESM::LuaScripts& data, bool resetScriptList)
    {
        std::map<std::string, Script> scriptsWithoutSavedData;
        if (resetScriptList)
        {
            removeAllScripts();
            for (const ESM::LuaScript& script : data.mScripts)
                addNewScript(script.mScriptPath);
        }
        else
            scriptsWithoutSavedData = mScripts;
        mSecondsTimersQueue.clear();
        mHoursTimersQueue.clear();
        for (const ESM::LuaScript& script : data.mScripts)
        {
            auto iter = mScripts.find(script.mScriptPath);
            if (iter == mScripts.end())
                continue;
            scriptsWithoutSavedData.erase(iter->first);
            iter->second.mHiddenData.get<sol::table>(TEMPORARY_TIMER_CALLBACKS).clear();
            try
            {
                sol::object handler = getFieldOrNil(iter->second.mInterface, ENGINE_HANDLERS, HANDLER_LOAD);
                if (handler != sol::nil)
                {
                    sol::object state = deserialize(mLua.sol(), script.mData, mSerializer);
                    LuaUtil::call(handler, state);
                }
            }
            catch (std::exception& e)
            {
                Log(Debug::Error) << mNamePrefix << "[" << script.mScriptPath << "] onLoad failed: " << e.what();
            }
            for (const ESM::LuaTimer& savedTimer : script.mTimers)
            {
                Timer timer;
                timer.mCallback = savedTimer.mCallbackName;
                timer.mSerializable = true;
                timer.mScript = script.mScriptPath;
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
                catch (std::exception& e)
                {
                    Log(Debug::Error) << mNamePrefix << "[" << script.mScriptPath << "] can not load timer: " << e.what();
                }
            }
        }
        for (auto& [path, script] : scriptsWithoutSavedData)
        {
            script.mHiddenData.get<sol::table>(TEMPORARY_TIMER_CALLBACKS).clear();
            sol::object handler = getFieldOrNil(script.mInterface, ENGINE_HANDLERS, HANDLER_LOAD);
            if (handler == sol::nil)
                continue;
            try { LuaUtil::call(handler); }
            catch (std::exception& e)
            {
                Log(Debug::Error) << mNamePrefix << "[" << path << "] onLoad failed: " << e.what();
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

    void ScriptsContainer::removeAllScripts()
    {
        for (auto& [_, script] : mScripts)
            script.mHiddenData[ScriptId::KEY] = sol::nil;
        mScripts.clear();
        mScriptOrder.clear();
        for (auto& [_, handlers] : mEngineHandlers)
            handlers->mList.clear();
        mEventHandlers.clear();
        mSecondsTimersQueue.clear();
        mHoursTimersQueue.clear();

        mPublicInterfaces.clear();
        // Assigned by LuaUtil::makeReadOnly, but `clear` removes it, so we need to assign it again.
        mPublicInterfaces[sol::meta_function::index] = mPublicInterfaces;
    }

    sol::table ScriptsContainer::getHiddenData(const std::string& scriptPath)
    {
        auto it = mScripts.find(scriptPath);
        if (it == mScripts.end())
            throw std::logic_error("ScriptsContainer::getHiddenData: script doesn't exist");
        return it->second.mHiddenData;
    }

    void ScriptsContainer::registerTimerCallback(const std::string& scriptPath, std::string_view callbackName, sol::function callback)
    {
        getHiddenData(scriptPath)[REGISTERED_TIMER_CALLBACKS][callbackName] = std::move(callback);
    }

    void ScriptsContainer::insertTimer(std::vector<Timer>& timerQueue, Timer&& t)
    {
        timerQueue.push_back(std::move(t));
        std::push_heap(timerQueue.begin(), timerQueue.end());
    }

    void ScriptsContainer::setupSerializableTimer(TimeUnit timeUnit, double time, const std::string& scriptPath,
                                                  std::string_view callbackName, sol::object callbackArg)
    {
        Timer t;
        t.mCallback = std::string(callbackName);
        t.mScript = scriptPath;
        t.mSerializable = true;
        t.mTime = time;
        t.mArg = callbackArg;
        t.mSerializedArg = serialize(t.mArg, mSerializer);
        insertTimer(timeUnit == TimeUnit::HOURS ? mHoursTimersQueue : mSecondsTimersQueue, std::move(t));
    }

    void ScriptsContainer::setupUnsavableTimer(TimeUnit timeUnit, double time, const std::string& scriptPath, sol::function callback)
    {
        Timer t;
        t.mScript = scriptPath;
        t.mSerializable = false;
        t.mTime = time;

        t.mCallback = mTemporaryCallbackCounter;
        getHiddenData(scriptPath)[TEMPORARY_TIMER_CALLBACKS][mTemporaryCallbackCounter] = std::move(callback);
        mTemporaryCallbackCounter++;

        insertTimer(timeUnit == TimeUnit::HOURS ? mHoursTimersQueue : mSecondsTimersQueue, std::move(t));
    }

    void ScriptsContainer::callTimer(const Timer& t)
    {
        try
        {
            sol::table data = getHiddenData(t.mScript);
            if (t.mSerializable)
            {
                const std::string& callbackName = std::get<std::string>(t.mCallback);
                sol::object callback = data[REGISTERED_TIMER_CALLBACKS][callbackName];
                if (!callback.is<sol::function>())
                    throw std::logic_error("Callback '" + callbackName + "' doesn't exist");
                LuaUtil::call(callback, t.mArg);
            }
            else
            {
                int64_t id = std::get<int64_t>(t.mCallback);
                sol::table callbacks = data[TEMPORARY_TIMER_CALLBACKS];
                sol::object callback = callbacks[id];
                if (!callback.is<sol::function>())
                    throw std::logic_error("Temporary timer callback doesn't exist");
                LuaUtil::call(callback);
                callbacks[id] = sol::nil;
            }
        }
        catch (std::exception& e)
        {
            Log(Debug::Error) << mNamePrefix << "[" << t.mScript << "] callTimer failed: " << e.what();
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

    void ScriptsContainer::processTimers(double gameSeconds, double gameHours)
    {
        updateTimerQueue(mSecondsTimersQueue, gameSeconds);
        updateTimerQueue(mHoursTimersQueue, gameHours);
    }

}
