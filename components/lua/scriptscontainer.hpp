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
// Scripts within one container can interact via interfaces (not implemented yet).
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
//             onSave = function() return ... end,
//             onLoad = function(state) ... end,  -- "state" is the data that was earlier returned by onSave
//
//             -- Works only if ScriptsContainer::registerEngineHandler is overloaded in a child class
//             -- and explicitly supports 'onSomethingElse'
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
        struct ScriptId
        {
            // ScriptId is stored in hidden data (see getHiddenData) with this key.
            constexpr static std::string_view KEY = "_id";

            ScriptsContainer* mContainer;
            std::string mPath;

            std::string toString() const;
        };
        using TimeUnit = ESM::LuaTimer::TimeUnit;

        // `namePrefix` is a common prefix for all scripts in the container. Used in logs for error messages and `print` output.
        ScriptsContainer(LuaUtil::LuaState* lua, std::string_view namePrefix);
        ScriptsContainer(const ScriptsContainer&) = delete;
        ScriptsContainer(ScriptsContainer&&) = delete;
        virtual ~ScriptsContainer();

        // Adds package that will be available (via `require`) for all scripts in the container.
        // Automatically applies LuaUtil::makeReadOnly to the package.
        void addPackage(const std::string& packageName, sol::object package);

        // Finds a file with given path in the virtual file system, starts as a new script, and adds it to the container.
        // Returns `true` if the script was successfully added. Otherwise prints an error message and returns `false`.
        // `false` can be returned if either file not found or has syntax errors or such script already exists in the container.
        bool addNewScript(const std::string& path);

        // Removes script. Returns `true` if it was successfully removed.
        bool removeScript(const std::string& path);
        void removeAllScripts();

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

        // Calls engineHandler "onSave" for every script and saves the list of the scripts with serialized data to ESM::LuaScripts.
        void save(ESM::LuaScripts&);

        // Calls engineHandler "onLoad" for every script with given data.
        // If resetScriptList=true, then removes all currently active scripts and runs the scripts that were saved in ESM::LuaScripts.
        // If resetScriptList=false, then list of running scripts is not changed, only engineHandlers "onLoad" are called.
        void load(const ESM::LuaScripts&, bool resetScriptList);

        // Returns the hidden data of a script.
        // Each script has a corresponding "hidden data" - a lua table that is not accessible from the script itself,
        // but can be used by built-in packages. It contains ScriptId and can contain any arbitrary data.
        sol::table getHiddenData(const std::string& scriptPath);

        // Callbacks for serializable timers should be registered in advance.
        // The script with the given path should already present in the container.
        void registerTimerCallback(const std::string& scriptPath, std::string_view callbackName, sol::function callback);

        // Sets up a timer, that can be automatically saved and loaded.
        //   timeUnit - game seconds (TimeUnit::Seconds) or game hours (TimeUnit::Hours).
        //   time - the absolute game time (in seconds or in hours) when the timer should be executed.
        //   scriptPath - script path in VFS is used as script id. The script with the given path should already present in the container.
        //   callbackName - callback (should be registered in advance) for this timer.
        //   callbackArg - parameter for the callback (should be serializable).
        void setupSerializableTimer(TimeUnit timeUnit, double time, const std::string& scriptPath,
                                    std::string_view callbackName, sol::object callbackArg);

        // Creates a timer. `callback` is an arbitrary Lua function. This type of timers is called "unsavable"
        // because it can not be stored in saves. I.e. loading a saved game will not fully restore the state.
        void setupUnsavableTimer(TimeUnit timeUnit, double time, const std::string& scriptPath, sol::function callback);

    protected:
        struct EngineHandlerList
        {
            std::string_view mName;
            std::vector<sol::protected_function> mList;

            // "name" must be string literal
            explicit EngineHandlerList(std::string_view name) : mName(name) {}
        };

        // Calls given handlers in direct order.
        template <typename... Args>
        void callEngineHandlers(EngineHandlerList& handlers, const Args&... args)
        {
            for (sol::protected_function& handler : handlers.mList)
            {
                try { LuaUtil::call(handler, args...); }
                catch (std::exception& e)
                {
                    Log(Debug::Error) << mNamePrefix << " " << handlers.mName << " failed. " << e.what();
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
            sol::object mInterface;  // returned value of the script (sol::table or nil)
            sol::table mHiddenData;
        };
        struct Timer
        {
            double mTime;
            bool mSerializable;
            std::string mScript;
            std::variant<std::string, int64_t> mCallback;  // string if serializable, integer otherwise
            sol::object mArg;
            std::string mSerializedArg;

            bool operator<(const Timer& t) const { return mTime > t.mTime; }
        };
        using EventHandlerList = std::vector<sol::protected_function>;

        void parseEngineHandlers(sol::table handlers, std::string_view scriptPath);
        void parseEventHandlers(sol::table handlers, std::string_view scriptPath);

        void callTimer(const Timer& t);
        void updateTimerQueue(std::vector<Timer>& timerQueue, double time);
        static void insertTimer(std::vector<Timer>& timerQueue, Timer&& t);

        const UserdataSerializer* mSerializer = nullptr;
        std::map<std::string, sol::object> API;

        std::vector<std::string> mScriptOrder;
        std::map<std::string, Script> mScripts;
        sol::table mPublicInterfaces;

        EngineHandlerList mUpdateHandlers{"onUpdate"};
        std::map<std::string_view, EngineHandlerList*> mEngineHandlers;
        std::map<std::string, EventHandlerList, std::less<>> mEventHandlers;

        std::vector<Timer> mSecondsTimersQueue;
        std::vector<Timer> mHoursTimersQueue;
        int64_t mTemporaryCallbackCounter = 0;
    };

}

#endif // COMPONENTS_LUA_SCRIPTSCONTAINER_H
