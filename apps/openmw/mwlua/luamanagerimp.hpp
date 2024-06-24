#ifndef MWLUA_LUAMANAGERIMP_H
#define MWLUA_LUAMANAGERIMP_H

#include <filesystem>
#include <map>
#include <osg/Stats>
#include <set>

#include <components/lua/inputactions.hpp>
#include <components/lua/luastate.hpp>
#include <components/lua/storage.hpp>
#include <components/lua_ui/resources.hpp>
#include <components/misc/color.hpp>

#include "../mwbase/luamanager.hpp"

#include "engineevents.hpp"
#include "globalscripts.hpp"
#include "localscripts.hpp"
#include "luaevents.hpp"
#include "menuscripts.hpp"
#include "object.hpp"
#include "objectlists.hpp"

namespace MWLua
{
    // \brief LuaManager is the central interface through which the engine invokes lua scripts.
    //
    // This class implements the interface defined in MWBase::LuaManager.
    // In addition to the interface, this class exposes lower level interaction between the engine
    // and the lua world.
    class LuaManager : public MWBase::LuaManager
    {
    public:
        LuaManager(const VFS::Manager* vfs, const std::filesystem::path& libsDir);
        LuaManager(const LuaManager&) = delete;
        LuaManager(LuaManager&&) = delete;
        ~LuaManager();

        // Called by engine.cpp when the environment is fully initialized.
        void init();

        void loadPermanentStorage(const std::filesystem::path& userConfigPath);
        void savePermanentStorage(const std::filesystem::path& userConfigPath);

        // \brief Executes lua handlers. Defaults to running in parallel with OSG Cull.
        //
        // The OSG Cull is expensive enough that we have "free" time to
        // execute Lua by running it in parallel. The Cull also does
        // not modify the game state, meaning we can safely read state from Lua
        // despite the concurrency. Only modifying the parts of the game state
        // that affect the scene graph is forbidden. Such modifications must
        // be queued for execution in synchronizedUpdate().
        // The parallelism can be turned off in the settings.
        void update();

        // \brief Executes latency-critical and scene graph related Lua logic.
        //
        // Called by engine.cpp from the main thread between InputManager and MechanicsManager updates.
        // Can use the scene graph and applies the actions queued during update()
        void synchronizedUpdate();

        // Normally it is called by `synchronizedUpdate` every frame.
        // Additionally must be called before making a save to ensure that there are no pending delayed
        // actions and the world is in a consistent state.
        void applyDelayedActions() override;

        // Available everywhere through the MWBase::LuaManager interface.
        // LuaManager queues these events and propagates to scripts on the next `update` call.
        void newGameStarted() override;
        void gameLoaded() override;
        void gameEnded() override;
        void noGame() override;
        void objectAddedToScene(const MWWorld::Ptr& ptr) override;
        void objectRemovedFromScene(const MWWorld::Ptr& ptr) override;
        void inputEvent(const InputEvent& event) override;
        void itemConsumed(const MWWorld::Ptr& consumable, const MWWorld::Ptr& actor) override
        {
            mEngineEvents.addToQueue(EngineEvents::OnConsume{ getId(actor), getId(consumable) });
        }
        void objectActivated(const MWWorld::Ptr& object, const MWWorld::Ptr& actor) override
        {
            mEngineEvents.addToQueue(EngineEvents::OnActivate{ getId(actor), getId(object) });
        }
        void useItem(const MWWorld::Ptr& object, const MWWorld::Ptr& actor, bool force) override;
        void animationTextKey(const MWWorld::Ptr& actor, const std::string& key) override;
        void playAnimation(const MWWorld::Ptr& actor, const std::string& groupname,
            const MWRender::AnimPriority& priority, int blendMask, bool autodisable, float speedmult,
            std::string_view start, std::string_view stop, float startpoint, uint32_t loops,
            bool loopfallback) override;
        void skillUse(const MWWorld::Ptr& actor, ESM::RefId skillId, int useType, float scale) override;
        void skillLevelUp(const MWWorld::Ptr& actor, ESM::RefId skillId, std::string_view source) override;
        void exteriorCreated(MWWorld::CellStore& cell) override
        {
            mEngineEvents.addToQueue(EngineEvents::OnNewExterior{ cell });
        }
        void objectTeleported(const MWWorld::Ptr& ptr) override;
        void questUpdated(const ESM::RefId& questId, int stage) override;
        void uiModeChanged(const MWWorld::Ptr& arg) override;
        void actorDied(const MWWorld::Ptr& actor) override;

        MWBase::LuaManager::ActorControls* getActorControls(const MWWorld::Ptr&) const override;

        void clear() override; // should be called before loading game or starting a new game to reset internal state.
        void setupPlayer(const MWWorld::Ptr& ptr) override; // Should be called once after each "clear".

        // Used only in Lua bindings
        void addCustomLocalScript(const MWWorld::Ptr&, int scriptId, std::string_view initData);
        void addUIMessage(std::string_view message) { mUIMessages.emplace_back(message); }
        void addInGameConsoleMessage(const std::string& msg, const Misc::Color& color)
        {
            mInGameConsoleMessages.push_back({ msg, color });
        }

        // Some changes to the game world can not be done from the scripting thread (because it runs in parallel with
        // OSG Cull), so we need to queue it and apply from the main thread.
        void addAction(std::function<void()> action, std::string_view name = "");
        void addTeleportPlayerAction(std::function<void()> action);

        // Saving
        void write(ESM::ESMWriter& writer, Loading::Listener& progress) override;
        void saveLocalScripts(const MWWorld::Ptr& ptr, ESM::LuaScripts& data) override;

        // Loading from a save
        void readRecord(ESM::ESMReader& reader, uint32_t type) override;
        void loadLocalScripts(const MWWorld::Ptr& ptr, const ESM::LuaScripts& data) override;
        void setContentFileMapping(const std::map<int, int>& mapping) override { mContentFileMapping = mapping; }

        // At the end of the next `synchronizedUpdate` drops script cache and reloads all scripts.
        // Calls `onSave` and `onLoad` for every script.
        void reloadAllScripts() override { mReloadAllScriptsRequested = true; }

        void handleConsoleCommand(
            const std::string& consoleMode, const std::string& command, const MWWorld::Ptr& selectedPtr) override;

        // Used to call Lua callbacks from C++
        void queueCallback(LuaUtil::Callback callback, sol::main_object arg)
        {
            mQueuedCallbacks.push_back({ std::move(callback), std::move(arg) });
        }

        // Wraps Lua callback into an std::function.
        // NOTE: Resulted function is not thread safe. Can not be used while LuaManager::update() or
        //       any other Lua-related function is running.
        template <class Arg>
        std::function<void(Arg)> wrapLuaCallback(const LuaUtil::Callback& c)
        {
            return
                [this, c](Arg arg) { this->queueCallback(c, sol::main_object(this->mLua.sol(), sol::in_place, arg)); };
        }

        LuaUi::ResourceManager* uiResourceManager() { return &mUiResourceManager; }

        bool isProcessingInputEvents() const { return mProcessingInputEvents; }

        void reportStats(unsigned int frameNumber, osg::Stats& stats) const;
        std::string formatResourceUsageStats() const override;

        void globalMWScriptCalled(
            const ESM::RefId& scriptName, const MWWorld::Ptr& target, const bool started) override;

        LuaUtil::InputAction::Registry& inputActions() { return mInputActions; }
        LuaUtil::InputTrigger::Registry& inputTriggers() { return mInputTriggers; }

    private:
        void initConfiguration();
        LocalScripts* createLocalScripts(const MWWorld::Ptr& ptr,
            std::optional<LuaUtil::ScriptIdsWithInitializationData> autoStartConf = std::nullopt);
        void reloadAllScriptsImpl();

        bool mInitialized = false;
        bool mGlobalScriptsStarted = false;
        bool mProcessingInputEvents = false;
        bool mApplyingDelayedActions = false;
        bool mNewGameStarted = false;
        bool mReloadAllScriptsRequested = false;
        LuaUtil::ScriptsConfiguration mConfiguration;
        LuaUtil::LuaState mLua;
        LuaUi::ResourceManager mUiResourceManager;
        std::map<std::string, sol::object> mLocalPackages;
        std::map<std::string, sol::object> mPlayerPackages;

        MenuScripts mMenuScripts{ &mLua };
        GlobalScripts mGlobalScripts{ &mLua };
        std::set<LocalScripts*> mActiveLocalScripts;
        std::vector<LocalScripts*> mQueuedAutoStartedScripts;
        ObjectLists mObjectLists;

        MWWorld::Ptr mPlayer;

        LuaEvents mLuaEvents{ mGlobalScripts, mMenuScripts };
        EngineEvents mEngineEvents{ mGlobalScripts };
        std::vector<MWBase::LuaManager::InputEvent> mInputEvents;
        std::vector<MWBase::LuaManager::InputEvent> mMenuInputEvents;

        std::unique_ptr<LuaUtil::UserdataSerializer> mGlobalSerializer;
        std::unique_ptr<LuaUtil::UserdataSerializer> mLocalSerializer;

        std::map<int, int> mContentFileMapping;
        std::unique_ptr<LuaUtil::UserdataSerializer> mGlobalLoader;
        std::unique_ptr<LuaUtil::UserdataSerializer> mLocalLoader;

        struct CallbackWithData
        {
            LuaUtil::Callback mCallback;
            sol::main_object mArg;
        };
        std::vector<CallbackWithData> mQueuedCallbacks;

        // Queued actions that should be done in main thread. Processed by applyQueuedChanges().
        class DelayedAction
        {
        public:
            DelayedAction(LuaUtil::LuaState* state, std::function<void()> fn, std::string_view name);
            void apply() const;

        private:
            std::string mCallerTraceback;
            std::function<void()> mFn;
            std::string mName;
        };
        std::vector<DelayedAction> mActionQueue;
        std::optional<DelayedAction> mTeleportPlayerAction;
        std::vector<std::string> mUIMessages;
        std::vector<std::pair<std::string, Misc::Color>> mInGameConsoleMessages;
        std::optional<ObjectId> mDelayedUiModeChangedArg;

        LuaUtil::LuaStorage mGlobalStorage{ mLua.sol() };
        LuaUtil::LuaStorage mPlayerStorage{ mLua.sol() };

        LuaUtil::InputAction::Registry mInputActions;
        LuaUtil::InputTrigger::Registry mInputTriggers;
    };

}

#endif // MWLUA_LUAMANAGERIMP_H
