#ifndef COMPONENTS_LUA_SCRIPTSCONTAINER_H
#define COMPONENTS_LUA_SCRIPTSCONTAINER_H

#include <map>
#include <set>
#include <string>

#include <components/debug/debuglog.hpp>
#include <components/esm/luascripts.hpp>

#include "luastate.hpp"
#include "serialization.hpp"

namespace LuaUtil
{

// ScriptsContainer is a base class for all scripts containers (LocalScripts,
// GlobalScripts, PlayerScripts, etc). Each script runs in a separate sandbox.
// Scripts from different containers can interact to each other only via events.
// Scripts within one container can interact via interfaces.
// All scripts from one container have the same set of API packages available.
//
// Each script should return a table in a specific format that describes its
// handlers and interfaces. Every section of the table is optional. Basic structure:
//
//     local function update(dt)
//         print("Update")
//     end
//
//     local function someEventHandler(eventData)
//         print("'SomeEvent' received")
//     end
//
//     return {
//         -- Provides interface for other scripts in the same container
//         interfaceName = "InterfaceName",
//         interface = {
//             someFunction = function() print("someFunction was called from another script") end,
//         },
//
//         -- Script interface for the engine. Not available for other script.
//         -- An error is printed if unknown handler is specified.
//         engineHandlers = {
//             onUpdate = update,
//             onInit = function(initData) ... end,  -- used when the script is just created (not loaded)
//             onSave = function() return ... end,
//             onLoad = function(state, initData) ... end,  -- "state" is the data that was earlier returned by onSave
//
//             -- Works only if a child class has passed a EngineHandlerList
//             -- for 'onSomethingElse' to ScriptsContainer::registerEngineHandlers.
//             onSomethingElse = function() print("something else") end
//         },
//
//         -- Handlers for events, sent from other scripts. Engine itself never sent events. Any name can be used for an event.
//         eventHandlers = {
//             SomeEvent = someEventHandler
//         }
//     }

    class ScriptsContainer
    {
    public:
        // ScriptId of each script is stored with this key in Script::mHiddenData.
        // Removed from mHiddenData when the script if removed.
        constexpr static std::string_view sScriptIdKey = "_id";

        // Debug identifier of each script is stored with this key in Script::mHiddenData.
        // Present in mHiddenData even after removal of the script from ScriptsContainer.
        constexpr static std::string_view sScriptDebugNameKey = "_name";

        struct ScriptId
        {
            ScriptsContainer* mContainer;
            int mIndex;  // index in LuaUtil::ScriptsConfiguration
        };
        using TimeUnit = ESM::LuaTimer::TimeUnit;

        // `namePrefix` is a common prefix for all scripts in the container. Used in logs for error messages and `print` output.
        // `autoStartMode` specifies the list of scripts that should be autostarted in this container; the list itself is
        //     stored in ScriptsConfiguration: lua->getConfiguration().getListByFlag(autoStartMode).
        ScriptsContainer(LuaState* lua, std::string_view namePrefix, ESM::LuaScriptCfg::Flags autoStartMode = 0);

        ScriptsContainer(const ScriptsContainer&) = delete;
        ScriptsContainer(ScriptsContainer&&) = delete;
        virtual ~ScriptsContainer();

        ESM::LuaScriptCfg::Flags getAutoStartMode() const { return mAutoStartMode; }

        // Adds package that will be available (via `require`) for all scripts in the container.
        // Automatically applies LuaUtil::makeReadOnly to the package.
        void addPackage(std::string packageName, sol::object package);

        // Gets script with given id from ScriptsConfiguration, finds the source in the virtual file system, starts as a new script,
        // adds it to the container, and calls onInit for this script. Returns `true` if the script was successfully added.
        // The script should have CUSTOM flag. If the flag is not set, or file not found, or has syntax errors, returns false.
        // If such script already exists in the container, then also returns false.
        bool addCustomScript(int scriptId);

        bool hasScript(int scriptId) const { return mScripts.count(scriptId) != 0; }
        void removeScript(int scriptId);

        // Processes timers. gameSeconds and gameHours are time (in seconds and in game hours) passed from the game start.
        void processTimers(double gameSeconds, double gameHours);

        // Calls `onUpdate` (if present) for every script in the container.
        // Handlers are called in the same order as scripts were added.
        void update(float dt) { callEngineHandlers(mUpdateHandlers, dt); }

        // Calls event handlers `eventName` (if present) for every script.
        // If several scripts register handlers for `eventName`, they are called in reverse order.
        // If some handler returns `false`, all remaining handlers are ignored. Any other return value
        // (including `nil`) has no effect.
        void receiveEvent(std::string_view eventName, std::string_view eventData);

        // Serializer defines how to serialize/deserialize userdata. If serializer is not provided,
        // only built-in types and types from util package can be serialized.
        void setSerializer(const UserdataSerializer* serializer) { mSerializer = serializer; }

        // Starts scripts according to `autoStartMode` and calls `onInit` for them. Not needed if `load` is used.
        void addAutoStartedScripts();

        // Removes all scripts including the auto started.
        void removeAllScripts();

        // Calls engineHandler "onSave" for every script and saves the list of the scripts with serialized data to ESM::LuaScripts.
        void save(ESM::LuaScripts&);

        // Removes all scripts; starts scripts according to `autoStartMode` and
        // loads the savedScripts. Runs "onLoad" for each script.
        void load(const ESM::LuaScripts& savedScripts);

        // Callbacks for serializable timers should be registered in advance.
        // The script with the given path should already present in the container.
        void registerTimerCallback(int scriptId, std::string_view callbackName, sol::function callback);

        // Sets up a timer, that can be automatically saved and loaded.
        //   timeUnit - game seconds (TimeUnit::Seconds) or game hours (TimeUnit::Hours).
        //   time - the absolute game time (in seconds or in hours) when the timer should be executed.
        //   scriptPath - script path in VFS is used as script id. The script with the given path should already present in the container.
        //   callbackName - callback (should be registered in advance) for this timer.
        //   callbackArg - parameter for the callback (should be serializable).
        void setupSerializableTimer(TimeUnit timeUnit, double time, int scriptId,
                                    std::string_view callbackName, sol::object callbackArg);

        // Creates a timer. `callback` is an arbitrary Lua function. This type of timers is called "unsavable"
        // because it can not be stored in saves. I.e. loading a saved game will not fully restore the state.
        void setupUnsavableTimer(TimeUnit timeUnit, double time, int scriptId, sol::function callback);

    protected:
        struct Handler
        {
            int mScriptId;
            sol::function mFn;
        };

        struct EngineHandlerList
        {
            std::string_view mName;
            std::vector<Handler> mList;

            // "name" must be string literal
            explicit EngineHandlerList(std::string_view name) : mName(name) {}
        };

        // Calls given handlers in direct order.
        template <typename... Args>
        void callEngineHandlers(EngineHandlerList& handlers, const Args&... args)
        {
            for (Handler& handler : handlers.mList)
            {
                try { LuaUtil::call(handler.mFn, args...); }
                catch (std::exception& e)
                {
                    Log(Debug::Error) << mNamePrefix << "[" << scriptPath(handler.mScriptId) << "] "
                                      << handlers.mName << " failed. " << e.what();
                }
            }
        }

        // To add a new engine handler a derived class should register the corresponding EngineHandlerList and define
        // a public function (see how ScriptsContainer::update is implemented) that calls `callEngineHandlers`.
        void registerEngineHandlers(std::initializer_list<EngineHandlerList*> handlers);

        const std::string mNamePrefix;
        LuaUtil::LuaState& mLua;

    private:
        struct Script
        {
            std::optional<sol::function> mOnSave;
            std::optional<sol::function> mOnOverride;
            std::optional<sol::table> mInterface;
            std::string mInterfaceName;
            sol::table mHiddenData;
            std::map<std::string, sol::function> mRegisteredCallbacks;
            std::map<int64_t, sol::function> mTemporaryCallbacks;
            std::string mPath;
        };
        struct Timer
        {
            double mTime;
            bool mSerializable;
            int mScriptId;
            std::variant<std::string, int64_t> mCallback;  // string if serializable, integer otherwise
            sol::object mArg;
            std::string mSerializedArg;

            bool operator<(const Timer& t) const { return mTime > t.mTime; }
        };
        using EventHandlerList = std::vector<Handler>;

        // Add to container without calling onInit/onLoad.
        bool addScript(int scriptId, std::optional<sol::function>& onInit, std::optional<sol::function>& onLoad);

        // Returns script by id (throws an exception if doesn't exist)
        Script& getScript(int scriptId);

        void printError(int scriptId, std::string_view msg, const std::exception& e);
        const std::string& scriptPath(int scriptId) const { return mLua.getConfiguration()[scriptId].mScriptPath; }
        void callOnInit(int scriptId, const sol::function& onInit);
        void callTimer(const Timer& t);
        void updateTimerQueue(std::vector<Timer>& timerQueue, double time);
        static void insertTimer(std::vector<Timer>& timerQueue, Timer&& t);
        static void insertHandler(std::vector<Handler>& list, int scriptId, sol::function fn);
        static void removeHandler(std::vector<Handler>& list, int scriptId);
        void insertInterface(int scriptId, const Script& script);
        void removeInterface(int scriptId, const Script& script);

        ESM::LuaScriptCfg::Flags mAutoStartMode;
        const UserdataSerializer* mSerializer = nullptr;
        std::map<std::string, sol::object> mAPI;

        std::map<int, Script> mScripts;
        sol::table mPublicInterfaces;

        EngineHandlerList mUpdateHandlers{"onUpdate"};
        std::map<std::string_view, EngineHandlerList*> mEngineHandlers;
        std::map<std::string, EventHandlerList, std::less<>> mEventHandlers;

        std::vector<Timer> mSecondsTimersQueue;
        std::vector<Timer> mHoursTimersQueue;
        int64_t mTemporaryCallbackCounter = 0;
    };

    // Wrapper for a single-argument Lua function.
    // Holds information about the script the function belongs to.
    // Needed to prevent callback calls if the script was removed.
    struct Callback
    {
        sol::function mFunc;
        sol::table mHiddenData;  // same object as Script::mHiddenData in ScriptsContainer

        void operator()(sol::object arg) const;
    };

}

#endif // COMPONENTS_LUA_SCRIPTSCONTAINER_H
