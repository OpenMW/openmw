#ifndef MWLUA_LUAMANAGERIMP_H
#define MWLUA_LUAMANAGERIMP_H

#include <map>
#include <set>

#include <components/lua/luastate.hpp>

#include "../mwbase/luamanager.hpp"

#include "actions.hpp"
#include "object.hpp"
#include "eventqueue.hpp"
#include "globalscripts.hpp"
#include "localscripts.hpp"
#include "playerscripts.hpp"
#include "worldview.hpp"

namespace MWLua
{

    // Wrapper for a single-argument Lua function.
    // Holds information about the script the function belongs to.
    // Needed to prevent callback calls if the script was removed.
    struct Callback
    {
        static constexpr std::string_view SCRIPT_NAME_KEY = "name";

        sol::function mFunc;
        sol::table mHiddenData;

        void operator()(sol::object arg) const;
    };

    class LuaManager : public MWBase::LuaManager
    {
    public:
        LuaManager(const VFS::Manager* vfs, const std::vector<std::string>& globalScriptLists);

        // Called by engine.cpp when environment is fully initialized.
        void init();

        // Called by engine.cpp every frame. For performance reasons it works in a separate
        // thread (in parallel with osg Cull). Can not use scene graph.
        void update(bool paused, float dt);

        // Called by engine.cpp from the main thread. Can use scene graph.
        void applyQueuedChanges();

        // Available everywhere through the MWBase::LuaManager interface.
        // LuaManager queues these events and propagates to scripts on the next `update` call.
        void newGameStarted() override { mGlobalScripts.newGameStarted(); }
        void objectAddedToScene(const MWWorld::Ptr& ptr) override;
        void objectRemovedFromScene(const MWWorld::Ptr& ptr) override;
        void registerObject(const MWWorld::Ptr& ptr) override;
        void deregisterObject(const MWWorld::Ptr& ptr) override;
        void inputEvent(const InputEvent& event) override { mInputEvents.push_back(event); }
        void appliedToObject(const MWWorld::Ptr& toPtr, std::string_view recordId, const MWWorld::Ptr& fromPtr) override;

        MWBase::LuaManager::ActorControls* getActorControls(const MWWorld::Ptr&) const override;

        void clear() override;  // should be called before loading game or starting a new game to reset internal state.
        void setupPlayer(const MWWorld::Ptr& ptr) override;  // Should be called once after each "clear".

        // Used only in luabindings
        void addLocalScript(const MWWorld::Ptr&, const std::string& scriptPath);
        void addAction(std::unique_ptr<Action>&& action) { mActionQueue.push_back(std::move(action)); }
        void addTeleportPlayerAction(std::unique_ptr<TeleportAction>&& action) { mTeleportPlayerAction = std::move(action); }
        void addUIMessage(std::string_view message) { mUIMessages.emplace_back(message); }

        // Saving
        void write(ESM::ESMWriter& writer, Loading::Listener& progress) override;
        void saveLocalScripts(const MWWorld::Ptr& ptr, ESM::LuaScripts& data) override;

        // Loading from a save
        void readRecord(ESM::ESMReader& reader, uint32_t type) override;
        void loadLocalScripts(const MWWorld::Ptr& ptr, const ESM::LuaScripts& data) override;
        void setContentFileMapping(const std::map<int, int>& mapping) override { mContentFileMapping = mapping; }

        // Drops script cache and reloads all scripts. Calls `onSave` and `onLoad` for every script.
        void reloadAllScripts() override;

        // Used to call Lua callbacks from C++
        void queueCallback(Callback callback, sol::object arg) { mQueuedCallbacks.push_back({std::move(callback), std::move(arg)}); }

        // Wraps Lua callback into an std::function.
        // NOTE: Resulted function is not thread safe. Can not be used while LuaManager::update() or
        //       any other Lua-related function is running.
        template <class Arg>
        std::function<void(Arg)> wrapLuaCallback(const Callback& c)
        {
            return [this, c](Arg arg) { this->queueCallback(c, sol::make_object(c.mFunc.lua_state(), arg)); };
        }

    private:
        LocalScripts* createLocalScripts(const MWWorld::Ptr& ptr);

        bool mInitialized = false;
        LuaUtil::LuaState mLua;
        sol::table mNearbyPackage;
        sol::table mUserInterfacePackage;
        sol::table mCameraPackage;
        sol::table mInputPackage;
        sol::table mLocalSettingsPackage;
        sol::table mPlayerSettingsPackage;

        std::vector<std::string> mGlobalScriptList;
        GlobalScripts mGlobalScripts{&mLua};
        std::set<LocalScripts*> mActiveLocalScripts;
        WorldView mWorldView;

        bool mPlayerChanged = false;
        MWWorld::Ptr mPlayer;

        GlobalEventQueue mGlobalEvents;
        LocalEventQueue mLocalEvents;

        std::unique_ptr<LuaUtil::UserdataSerializer> mGlobalSerializer;
        std::unique_ptr<LuaUtil::UserdataSerializer> mLocalSerializer;

        std::map<int, int> mContentFileMapping;
        std::unique_ptr<LuaUtil::UserdataSerializer> mGlobalLoader;
        std::unique_ptr<LuaUtil::UserdataSerializer> mLocalLoader;

        std::vector<MWBase::LuaManager::InputEvent> mInputEvents;
        std::vector<ObjectId> mActorAddedEvents;

        struct CallbackWithData
        {
            Callback mCallback;
            sol::object mArg;
        };
        std::vector<CallbackWithData> mQueuedCallbacks;

        struct LocalEngineEvent
        {
            ObjectId mDest;
            LocalScripts::EngineEvent mEvent;
        };
        std::vector<LocalEngineEvent> mLocalEngineEvents;

        // Queued actions that should be done in main thread. Processed by applyQueuedChanges().
        std::vector<std::unique_ptr<Action>> mActionQueue;
        std::unique_ptr<TeleportAction> mTeleportPlayerAction;
        std::vector<std::string> mUIMessages;
    };

}

#endif  // MWLUA_LUAMANAGERIMP_H
