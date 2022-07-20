#ifndef MWLUA_LUAMANAGERIMP_H
#define MWLUA_LUAMANAGERIMP_H

#include <map>
#include <set>

#include <components/lua/l10n.hpp>
#include <components/lua/luastate.hpp>
#include <components/lua/storage.hpp>

#include <components/lua_ui/resources.hpp>

#include <components/misc/color.hpp>

#include "../mwbase/luamanager.hpp"

#include "object.hpp"
#include "eventqueue.hpp"
#include "globalscripts.hpp"
#include "localscripts.hpp"
#include "playerscripts.hpp"
#include "worldview.hpp"

namespace MWLua
{

    class LuaManager : public MWBase::LuaManager
    {
    public:
        LuaManager(const VFS::Manager* vfs, const std::string& libsDir);

        // Called by engine.cpp before UI setup.
        void initL10n();

        // Called by engine.cpp when the environment is fully initialized.
        void init();

        void loadPermanentStorage(const std::string& userConfigPath);
        void savePermanentStorage(const std::string& userConfigPath);

        // Called by engine.cpp every frame. For performance reasons it works in a separate
        // thread (in parallel with osg Cull). Can not use scene graph.
        void update();

        std::string translate(const std::string& contextName, const std::string& key) override;

        // Called by engine.cpp from the main thread. Can use scene graph.
        void synchronizedUpdate();

        // Available everywhere through the MWBase::LuaManager interface.
        // LuaManager queues these events and propagates to scripts on the next `update` call.
        void newGameStarted() override;
        void gameLoaded() override;
        void objectAddedToScene(const MWWorld::Ptr& ptr) override;
        void objectRemovedFromScene(const MWWorld::Ptr& ptr) override;
        void registerObject(const MWWorld::Ptr& ptr) override;
        void deregisterObject(const MWWorld::Ptr& ptr) override;
        void inputEvent(const InputEvent& event) override { mInputEvents.push_back(event); }
        void itemConsumed(const MWWorld::Ptr& consumable, const MWWorld::Ptr& actor) override;
        void objectActivated(const MWWorld::Ptr& object, const MWWorld::Ptr& actor) override;

        MWBase::LuaManager::ActorControls* getActorControls(const MWWorld::Ptr&) const override;

        void clear() override;  // should be called before loading game or starting a new game to reset internal state.
        void setupPlayer(const MWWorld::Ptr& ptr) override;  // Should be called once after each "clear".

        // Used only in Lua bindings
        void addCustomLocalScript(const MWWorld::Ptr&, int scriptId);
        void addUIMessage(std::string_view message) { mUIMessages.emplace_back(message); }
        void addInGameConsoleMessage(const std::string& msg, const Misc::Color& color)
        {
            mInGameConsoleMessages.push_back({msg, color});
        }
        
        // Some changes to the game world can not be done from the scripting thread (because it runs in parallel with OSG Cull),
        // so we need to queue it and apply from the main thread. All such changes should be implemented as classes inherited
        // from MWLua::Action.
        class Action
        {
        public:
            Action(LuaUtil::LuaState* state);
            virtual ~Action() {}

            void safeApply(WorldView&) const;
            virtual void apply(WorldView&) const = 0;
            virtual std::string toString() const = 0;

        private:
            std::string mCallerTraceback;
        };

        void addAction(std::function<void()> action, std::string_view name = "");
        void addAction(std::unique_ptr<Action>&& action) { mActionQueue.push_back(std::move(action)); }
        void addTeleportPlayerAction(std::unique_ptr<Action>&& action) { mTeleportPlayerAction = std::move(action); }

        // Saving
        void write(ESM::ESMWriter& writer, Loading::Listener& progress) override;
        void saveLocalScripts(const MWWorld::Ptr& ptr, ESM::LuaScripts& data) override;

        // Loading from a save
        void readRecord(ESM::ESMReader& reader, uint32_t type) override;
        void loadLocalScripts(const MWWorld::Ptr& ptr, const ESM::LuaScripts& data) override;
        void setContentFileMapping(const std::map<int, int>& mapping) override { mContentFileMapping = mapping; }

        // Drops script cache and reloads all scripts. Calls `onSave` and `onLoad` for every script.
        void reloadAllScripts() override;

        void handleConsoleCommand(const std::string& consoleMode, const std::string& command, const MWWorld::Ptr& selectedPtr) override;

        // Used to call Lua callbacks from C++
        void queueCallback(LuaUtil::Callback callback, sol::object arg)
        {
            mQueuedCallbacks.push_back({std::move(callback), std::move(arg)});
        }

        // Wraps Lua callback into an std::function.
        // NOTE: Resulted function is not thread safe. Can not be used while LuaManager::update() or
        //       any other Lua-related function is running.
        template <class Arg>
        std::function<void(Arg)> wrapLuaCallback(const LuaUtil::Callback& c)
        {
            return [this, c](Arg arg) { this->queueCallback(c, sol::make_object(c.mFunc.lua_state(), arg)); };
        }

        LuaUi::ResourceManager* uiResourceManager() { return &mUiResourceManager; }

        bool isProcessingInputEvents() const { return mProcessingInputEvents; }

    private:
        void initConfiguration();
        LocalScripts* createLocalScripts(const MWWorld::Ptr& ptr,
                                         std::optional<LuaUtil::ScriptIdsWithInitializationData> autoStartConf = std::nullopt);

        bool mInitialized = false;
        bool mGlobalScriptsStarted = false;
        bool mProcessingInputEvents = false;
        LuaUtil::ScriptsConfiguration mConfiguration;
        LuaUtil::LuaState mLua;
        LuaUi::ResourceManager mUiResourceManager;
        LuaUtil::L10nManager mL10n;
        sol::table mNearbyPackage;
        sol::table mUserInterfacePackage;
        sol::table mCameraPackage;
        sol::table mInputPackage;
        sol::table mLocalStoragePackage;
        sol::table mPlayerStoragePackage;
        sol::table mPostprocessingPackage;
        sol::table mDebugPackage;

        GlobalScripts mGlobalScripts{&mLua};
        std::set<LocalScripts*> mActiveLocalScripts;
        WorldView mWorldView;

        bool mPlayerChanged = false;
        bool mNewGameStarted = false;
        MWWorld::Ptr mPlayer;

        GlobalEventQueue mGlobalEvents;
        LocalEventQueue mLocalEvents;

        std::unique_ptr<LuaUtil::UserdataSerializer> mGlobalSerializer;
        std::unique_ptr<LuaUtil::UserdataSerializer> mLocalSerializer;

        std::map<int, int> mContentFileMapping;
        std::unique_ptr<LuaUtil::UserdataSerializer> mGlobalLoader;
        std::unique_ptr<LuaUtil::UserdataSerializer> mLocalLoader;

        std::vector<MWBase::LuaManager::InputEvent> mInputEvents;
        std::vector<ObjectId> mObjectAddedEvents;

        struct CallbackWithData
        {
            LuaUtil::Callback mCallback;
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
        std::unique_ptr<Action> mTeleportPlayerAction;
        std::vector<std::string> mUIMessages;
        std::vector<std::pair<std::string, Misc::Color>> mInGameConsoleMessages;

        LuaUtil::LuaStorage mGlobalStorage{mLua.sol()};
        LuaUtil::LuaStorage mPlayerStorage{mLua.sol()};
    };

}

#endif  // MWLUA_LUAMANAGERIMP_H
