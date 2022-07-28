#include "luamanagerimp.hpp"

#include <filesystem>

#include <components/debug/debuglog.hpp>

#include <components/esm3/esmreader.hpp>
#include <components/esm3/esmwriter.hpp>
#include <components/esm/luascripts.hpp>

#include <components/settings/settings.hpp>

#include <components/lua/utilpackage.hpp>

#include <components/lua_ui/util.hpp>

#include "../mwbase/windowmanager.hpp"

#include "../mwrender/postprocessor.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/ptr.hpp"

#include "luabindings.hpp"
#include "userdataserializer.hpp"
#include "types/types.hpp"
#include "debugbindings.hpp"

namespace MWLua
{

    LuaManager::LuaManager(const VFS::Manager* vfs, const std::string& libsDir)
        : mLua(vfs, &mConfiguration)
        , mUiResourceManager(vfs)
        , mL10n(vfs, &mLua)
    {
        Log(Debug::Info) << "Lua version: " << LuaUtil::getLuaVersion();
        mLua.addInternalLibSearchPath(libsDir);

        mGlobalSerializer = createUserdataSerializer(false, mWorldView.getObjectRegistry());
        mLocalSerializer = createUserdataSerializer(true, mWorldView.getObjectRegistry());
        mGlobalLoader = createUserdataSerializer(false, mWorldView.getObjectRegistry(), &mContentFileMapping);
        mLocalLoader = createUserdataSerializer(true, mWorldView.getObjectRegistry(), &mContentFileMapping);

        mGlobalScripts.setSerializer(mGlobalSerializer.get());
    }

    void LuaManager::initConfiguration()
    {
        mConfiguration.init(MWBase::Environment::get().getWorld()->getStore().getLuaScriptsCfg());
        Log(Debug::Verbose) << "Lua scripts configuration (" << mConfiguration.size() << " scripts):";
        for (size_t i = 0; i < mConfiguration.size(); ++i)
            Log(Debug::Verbose) << "#" << i << " " << LuaUtil::scriptCfgToString(mConfiguration[i]);
        mGlobalScripts.setAutoStartConf(mConfiguration.getGlobalConf());
    }

    void LuaManager::initL10n()
    {
        mL10n.init();
        mL10n.setPreferredLocales(Settings::Manager::getStringArray("preferred locales", "General"));
    }

    void LuaManager::init()
    {
        Context context;
        context.mIsGlobal = true;
        context.mLuaManager = this;
        context.mLua = &mLua;
        context.mL10n = &mL10n;
        context.mWorldView = &mWorldView;
        context.mLocalEventQueue = &mLocalEvents;
        context.mGlobalEventQueue = &mGlobalEvents;
        context.mSerializer = mGlobalSerializer.get();

        Context localContext = context;
        localContext.mIsGlobal = false;
        localContext.mSerializer = mLocalSerializer.get();

        initObjectBindingsForGlobalScripts(context);
        initCellBindingsForGlobalScripts(context);
        initObjectBindingsForLocalScripts(localContext);
        initCellBindingsForLocalScripts(localContext);
        LocalScripts::initializeSelfPackage(localContext);
        LuaUtil::LuaStorage::initLuaBindings(mLua.sol());

        mLua.addCommonPackage("openmw.async", getAsyncPackageInitializer(context));
        mLua.addCommonPackage("openmw.util", LuaUtil::initUtilPackage(mLua.sol()));
        mLua.addCommonPackage("openmw.core", initCorePackage(context));
        mLua.addCommonPackage("openmw.types", initTypesPackage(context));
        mGlobalScripts.addPackage("openmw.world", initWorldPackage(context));
        mGlobalScripts.addPackage("openmw.storage", initGlobalStoragePackage(context, &mGlobalStorage));

        mCameraPackage = initCameraPackage(localContext);
        mUserInterfacePackage = initUserInterfacePackage(localContext);
        mInputPackage = initInputPackage(localContext);
        mNearbyPackage = initNearbyPackage(localContext);
        mLocalStoragePackage = initLocalStoragePackage(localContext, &mGlobalStorage);
        mPlayerStoragePackage = initPlayerStoragePackage(localContext, &mGlobalStorage, &mPlayerStorage);
        mPostprocessingPackage = initPostprocessingPackage(localContext);
        mDebugPackage = initDebugPackage(localContext);

        initConfiguration();
        mInitialized = true;
    }

    std::string LuaManager::translate(const std::string& contextName, const std::string& key)
    {
        return mL10n.translate(contextName, key);
    }

    void LuaManager::loadPermanentStorage(const std::string& userConfigPath)
    {
        auto globalPath = std::filesystem::path(userConfigPath) / "global_storage.bin";
        auto playerPath = std::filesystem::path(userConfigPath) / "player_storage.bin";
        if (std::filesystem::exists(globalPath))
            mGlobalStorage.load(globalPath.string());
        if (std::filesystem::exists(playerPath))
            mPlayerStorage.load(playerPath.string());
    }

    void LuaManager::savePermanentStorage(const std::string& userConfigPath)
    {
        std::filesystem::path confDir(userConfigPath);
        mGlobalStorage.save((confDir / "global_storage.bin").string());
        mPlayerStorage.save((confDir / "player_storage.bin").string());
    }

    void LuaManager::update()
    {
        static const bool luaDebug = Settings::Manager::getBool("lua debug", "Lua");
        if (mPlayer.isEmpty())
            return;  // The game is not started yet.

        float frameDuration = MWBase::Environment::get().getFrameDuration();
        ObjectRegistry* objectRegistry = mWorldView.getObjectRegistry();

        MWWorld::Ptr newPlayerPtr = MWBase::Environment::get().getWorld()->getPlayerPtr();
        if (!(getId(mPlayer) == getId(newPlayerPtr)))
            throw std::logic_error("Player Refnum was changed unexpectedly");
        if (!mPlayer.isInCell() || !newPlayerPtr.isInCell() || mPlayer.getCell() != newPlayerPtr.getCell())
        {
            mPlayer = newPlayerPtr;  // player was moved to another cell, update ptr in registry
            objectRegistry->registerPtr(mPlayer);
        }

        mWorldView.update();

        std::vector<GlobalEvent> globalEvents = std::move(mGlobalEvents);
        std::vector<LocalEvent> localEvents = std::move(mLocalEvents);
        mGlobalEvents = std::vector<GlobalEvent>();
        mLocalEvents = std::vector<LocalEvent>();

        if (!mWorldView.isPaused())
        {  // Update time and process timers
            double simulationTime = mWorldView.getSimulationTime() + frameDuration;
            mWorldView.setSimulationTime(simulationTime);
            double gameTime = mWorldView.getGameTime();

            mGlobalScripts.processTimers(simulationTime, gameTime);
            for (LocalScripts* scripts : mActiveLocalScripts)
                scripts->processTimers(simulationTime, gameTime);
        }

        // Receive events
        for (GlobalEvent& e : globalEvents)
            mGlobalScripts.receiveEvent(e.mEventName, e.mEventData);
        for (LocalEvent& e : localEvents)
        {
            LObject obj(e.mDest, objectRegistry);
            LocalScripts* scripts = obj.isValid() ? obj.ptr().getRefData().getLuaScripts() : nullptr;
            if (scripts)
                scripts->receiveEvent(e.mEventName, e.mEventData);
            else
                Log(Debug::Debug) << "Ignored event " << e.mEventName << " to L" << idToString(e.mDest)
                                  << ". Object not found or has no attached scripts";
        }

        // Run queued callbacks
        for (CallbackWithData& c : mQueuedCallbacks)
            c.mCallback.call(c.mArg);
        mQueuedCallbacks.clear();

        // Engine handlers in local scripts
        for (const LocalEngineEvent& e : mLocalEngineEvents)
        {
            LObject obj(e.mDest, objectRegistry);
            if (!obj.isValid())
            {
                if (luaDebug)
                    Log(Debug::Verbose) << "Can not call engine handlers: object" << idToString(e.mDest) << " is not found";
                continue;
            }
            LocalScripts* scripts = obj.ptr().getRefData().getLuaScripts();
            if (scripts)
                scripts->receiveEngineEvent(e.mEvent);
        }
        mLocalEngineEvents.clear();

        if (!mWorldView.isPaused())
        {
            for (LocalScripts* scripts : mActiveLocalScripts)
                scripts->update(frameDuration);
        }

        // Engine handlers in global scripts
        if (mPlayerChanged)
        {
            mPlayerChanged = false;
            mGlobalScripts.playerAdded(GObject(getId(mPlayer), objectRegistry));
        }
        if (mNewGameStarted)
        {
            mNewGameStarted = false;
            mGlobalScripts.newGameStarted();
        }

        for (ObjectId id : mObjectAddedEvents)
        {
            GObject obj(id, objectRegistry);
            if (obj.isValid())
            {
                mGlobalScripts.objectActive(obj);
                const MWWorld::Class& objClass = obj.ptr().getClass();
                if (objClass.isActor())
                    mGlobalScripts.actorActive(obj);
                if (mWorldView.isItem(obj.ptr()))
                    mGlobalScripts.itemActive(obj);
            }
            else if (luaDebug)
                Log(Debug::Verbose) << "Could not resolve a Lua object added event: object" << idToString(id)
                    << " is already removed";
        }
        mObjectAddedEvents.clear();

        if (!mWorldView.isPaused())
            mGlobalScripts.update(frameDuration);
    }

    void LuaManager::synchronizedUpdate()
    {
        if (mPlayer.isEmpty())
            return;  // The game is not started yet.

        // We apply input events in `synchronizedUpdate` rather than in `update` in order to reduce input latency.
        mProcessingInputEvents = true;
        PlayerScripts* playerScripts = dynamic_cast<PlayerScripts*>(mPlayer.getRefData().getLuaScripts());
        if (playerScripts && !MWBase::Environment::get().getWindowManager()->containsMode(MWGui::GM_MainMenu))
        {
            for (const auto& event : mInputEvents)
                playerScripts->processInputEvent(event);
        }
        mInputEvents.clear();
        if (playerScripts)
            playerScripts->onFrame(mWorldView.isPaused() ? 0.0 : MWBase::Environment::get().getFrameDuration());
        mProcessingInputEvents = false;

        MWBase::WindowManager* windowManager = MWBase::Environment::get().getWindowManager();
        for (const std::string& message : mUIMessages)
            windowManager->messageBox(message);
        mUIMessages.clear();
        for (auto& [msg, color] : mInGameConsoleMessages)
            windowManager->printToConsole(msg, "#" + color.toHex());
        mInGameConsoleMessages.clear();

        for (std::unique_ptr<Action>& action : mActionQueue)
            action->safeApply(mWorldView);
        mActionQueue.clear();
        
        if (mTeleportPlayerAction)
            mTeleportPlayerAction->safeApply(mWorldView);
        mTeleportPlayerAction.reset();
    }

    void LuaManager::clear()
    {
        LuaUi::clearUserInterface();
        mUiResourceManager.clear();
        MWBase::Environment::get().getWindowManager()->setConsoleMode("");
        MWBase::Environment::get().getWorld()->getPostProcessor()->disableDynamicShaders();
        mActiveLocalScripts.clear();
        mLocalEvents.clear();
        mGlobalEvents.clear();
        mInputEvents.clear();
        mObjectAddedEvents.clear();
        mLocalEngineEvents.clear();
        mNewGameStarted = false;
        mPlayerChanged = false;
        mWorldView.clear();
        mGlobalScripts.removeAllScripts();
        mGlobalScriptsStarted = false;
        if (!mPlayer.isEmpty())
        {
            mPlayer.getCellRef().unsetRefNum();
            mPlayer.getRefData().setLuaScripts(nullptr);
            mPlayer = MWWorld::Ptr();
        }
        mGlobalStorage.clearTemporaryAndRemoveCallbacks();
        mPlayerStorage.clearTemporaryAndRemoveCallbacks();
    }

    void LuaManager::setupPlayer(const MWWorld::Ptr& ptr)
    {
        if (!mInitialized)
            return;
        if (!mPlayer.isEmpty())
            throw std::logic_error("Player is initialized twice");
        mWorldView.objectAddedToScene(ptr);
        mPlayer = ptr;
        LocalScripts* localScripts = ptr.getRefData().getLuaScripts();
        if (!localScripts)
        {
            localScripts = createLocalScripts(ptr);
            localScripts->addAutoStartedScripts();
        }
        mActiveLocalScripts.insert(localScripts);
        mLocalEngineEvents.push_back({getId(ptr), LocalScripts::OnActive{}});
        mPlayerChanged = true;
    }

    void LuaManager::newGameStarted()
    {
        mNewGameStarted = true;
        mInputEvents.clear();
        mGlobalScripts.addAutoStartedScripts();
        mGlobalScriptsStarted = true;
    }

    void LuaManager::gameLoaded()
    {
        if (!mGlobalScriptsStarted)
            mGlobalScripts.addAutoStartedScripts();
        mGlobalScriptsStarted = true;
    }

    void LuaManager::objectAddedToScene(const MWWorld::Ptr& ptr)
    {
        mWorldView.objectAddedToScene(ptr);  // assigns generated RefNum if it is not set yet.

        LocalScripts* localScripts = ptr.getRefData().getLuaScripts();
        if (!localScripts)
        {
            LuaUtil::ScriptIdsWithInitializationData autoStartConf =
                mConfiguration.getLocalConf(getLiveCellRefType(ptr.mRef), ptr.getCellRef().getRefId(), getId(ptr));
            if (!autoStartConf.empty())
            {
                localScripts = createLocalScripts(ptr, std::move(autoStartConf));
                localScripts->addAutoStartedScripts();  // TODO: put to a queue and apply on next `update()`
            }
        }
        if (localScripts)
        {
            mActiveLocalScripts.insert(localScripts);
            mLocalEngineEvents.push_back({getId(ptr), LocalScripts::OnActive{}});
        }

        if (ptr != mPlayer)
            mObjectAddedEvents.push_back(getId(ptr));
    }

    void LuaManager::objectRemovedFromScene(const MWWorld::Ptr& ptr)
    {
        mWorldView.objectRemovedFromScene(ptr);
        LocalScripts* localScripts = ptr.getRefData().getLuaScripts();
        if (localScripts)
        {
            mActiveLocalScripts.erase(localScripts);
            if (!mWorldView.getObjectRegistry()->getPtr(getId(ptr), true).isEmpty())
                mLocalEngineEvents.push_back({getId(ptr), LocalScripts::OnInactive{}});
        }
    }

    void LuaManager::registerObject(const MWWorld::Ptr& ptr)
    {
        mWorldView.getObjectRegistry()->registerPtr(ptr);
    }

    void LuaManager::deregisterObject(const MWWorld::Ptr& ptr)
    {
        mWorldView.getObjectRegistry()->deregisterPtr(ptr);
    }

    void LuaManager::itemConsumed(const MWWorld::Ptr& consumable, const MWWorld::Ptr& actor)
    {
        mWorldView.getObjectRegistry()->registerPtr(consumable);
        mLocalEngineEvents.push_back({getId(actor), LocalScripts::OnConsume{LObject(getId(consumable), mWorldView.getObjectRegistry())}});
    }

    void LuaManager::objectActivated(const MWWorld::Ptr& object, const MWWorld::Ptr& actor)
    {
        mLocalEngineEvents.push_back({getId(object), LocalScripts::OnActivated{LObject(getId(actor), mWorldView.getObjectRegistry())}});
    }

    MWBase::LuaManager::ActorControls* LuaManager::getActorControls(const MWWorld::Ptr& ptr) const
    {
        LocalScripts* localScripts = ptr.getRefData().getLuaScripts();
        if (!localScripts)
            return nullptr;
        return localScripts->getActorControls();
    }

    void LuaManager::addCustomLocalScript(const MWWorld::Ptr& ptr, int scriptId)
    {
        LocalScripts* localScripts = ptr.getRefData().getLuaScripts();
        if (!localScripts)
        {
            localScripts = createLocalScripts(ptr);
            localScripts->addAutoStartedScripts();
            if (ptr.isInCell() && MWBase::Environment::get().getWorld()->isCellActive(ptr.getCell()))
                mActiveLocalScripts.insert(localScripts);
        }
        localScripts->addCustomScript(scriptId);
    }

    LocalScripts* LuaManager::createLocalScripts(const MWWorld::Ptr& ptr,
                                                 std::optional<LuaUtil::ScriptIdsWithInitializationData> autoStartConf)
    {
        assert(mInitialized);
        std::shared_ptr<LocalScripts> scripts;
        const uint32_t type = getLiveCellRefType(ptr.mRef);
        if (type == ESM::REC_STAT)
            throw std::runtime_error("Lua scripts on static objects are not allowed");
        else if (type == ESM::REC_INTERNAL_PLAYER)
        {
            scripts = std::make_shared<PlayerScripts>(&mLua, LObject(getId(ptr), mWorldView.getObjectRegistry()));
            scripts->setAutoStartConf(mConfiguration.getPlayerConf());
            scripts->addPackage("openmw.ui", mUserInterfacePackage);
            scripts->addPackage("openmw.camera", mCameraPackage);
            scripts->addPackage("openmw.input", mInputPackage);
            scripts->addPackage("openmw.storage", mPlayerStoragePackage);
            scripts->addPackage("openmw.postprocessing", mPostprocessingPackage);
            scripts->addPackage("openmw.debug", mDebugPackage);
        }
        else
        {
            scripts = std::make_shared<LocalScripts>(&mLua, LObject(getId(ptr), mWorldView.getObjectRegistry()));
            if (!autoStartConf.has_value())
                autoStartConf = mConfiguration.getLocalConf(type, ptr.getCellRef().getRefId(), getId(ptr));
            scripts->setAutoStartConf(std::move(*autoStartConf));
            scripts->addPackage("openmw.storage", mLocalStoragePackage);
        }
        scripts->addPackage("openmw.nearby", mNearbyPackage);
        scripts->setSerializer(mLocalSerializer.get());

        MWWorld::RefData& refData = ptr.getRefData();
        refData.setLuaScripts(std::move(scripts));
        return refData.getLuaScripts();
    }

    void LuaManager::write(ESM::ESMWriter& writer, Loading::Listener& progress)
    {
        writer.startRecord(ESM::REC_LUAM);

        mWorldView.save(writer);
        ESM::LuaScripts globalScripts;
        mGlobalScripts.save(globalScripts);
        globalScripts.save(writer);
        saveEvents(writer, mGlobalEvents, mLocalEvents);

        writer.endRecord(ESM::REC_LUAM);
    }

    void LuaManager::readRecord(ESM::ESMReader& reader, uint32_t type)
    {
        if (type != ESM::REC_LUAM)
            throw std::runtime_error("ESM::REC_LUAM is expected");

        mWorldView.load(reader);
        ESM::LuaScripts globalScripts;
        globalScripts.load(reader);
        loadEvents(mLua.sol(), reader, mGlobalEvents, mLocalEvents, mContentFileMapping, mGlobalLoader.get());

        mGlobalScripts.setSavedDataDeserializer(mGlobalLoader.get());
        mGlobalScripts.load(globalScripts);
        mGlobalScriptsStarted = true;
    }

    void LuaManager::saveLocalScripts(const MWWorld::Ptr& ptr, ESM::LuaScripts& data)
    {
        if (ptr.getRefData().getLuaScripts())
            ptr.getRefData().getLuaScripts()->save(data);
        else
            data.mScripts.clear();
    }

    void LuaManager::loadLocalScripts(const MWWorld::Ptr& ptr, const ESM::LuaScripts& data)
    {
        if (data.mScripts.empty())
        {
            if (ptr.getRefData().getLuaScripts())
                ptr.getRefData().setLuaScripts(nullptr);
            return;
        }

        mWorldView.getObjectRegistry()->registerPtr(ptr);
        LocalScripts* scripts = createLocalScripts(ptr);

        scripts->setSerializer(mLocalSerializer.get());
        scripts->setSavedDataDeserializer(mLocalLoader.get());
        scripts->load(data);

        // LiveCellRef is usually copied after loading, so this Ptr will become invalid and should be deregistered.
        mWorldView.getObjectRegistry()->deregisterPtr(ptr);
    }

    void LuaManager::reloadAllScripts()
    {
        Log(Debug::Info) << "Reload Lua";

        LuaUi::clearUserInterface();
        MWBase::Environment::get().getWindowManager()->setConsoleMode("");
        mUiResourceManager.clear();
        mLua.dropScriptCache();
        mL10n.clear();
        initConfiguration();

        {  // Reload global scripts
            mGlobalScripts.setSavedDataDeserializer(mGlobalSerializer.get());
            ESM::LuaScripts data;
            mGlobalScripts.save(data);
            mGlobalScripts.load(data);
        }

        for (const auto& [id, ptr] : mWorldView.getObjectRegistry()->mObjectMapping)
        {  // Reload local scripts
            LocalScripts* scripts = ptr.getRefData().getLuaScripts();
            if (scripts == nullptr)
                continue;
            scripts->setSavedDataDeserializer(mLocalSerializer.get());
            ESM::LuaScripts data;
            scripts->save(data);
            scripts->load(data);
        }
        for (LocalScripts* scripts : mActiveLocalScripts)
            scripts->receiveEngineEvent(LocalScripts::OnActive());
    }

    void LuaManager::handleConsoleCommand(const std::string& consoleMode, const std::string& command, const MWWorld::Ptr& selectedPtr)
    {
        PlayerScripts* playerScripts = nullptr;
        if (!mPlayer.isEmpty())
            playerScripts = dynamic_cast<PlayerScripts*>(mPlayer.getRefData().getLuaScripts());
        if (!playerScripts)
        {
            MWBase::Environment::get().getWindowManager()->printToConsole("You must enter a game session to run Lua commands\n",
                                                                          MWBase::WindowManager::sConsoleColor_Error);
            return;
        }
        sol::object selected = sol::nil;
        if (!selectedPtr.isEmpty())
            selected = sol::make_object(mLua.sol(), LObject(getId(selectedPtr), mWorldView.getObjectRegistry()));
        if (!playerScripts->consoleCommand(consoleMode, command, selected))
            MWBase::Environment::get().getWindowManager()->printToConsole("No Lua handlers for console\n",
                                                                          MWBase::WindowManager::sConsoleColor_Error);
    }

    LuaManager::Action::Action(LuaUtil::LuaState* state)
    {
        static const bool luaDebug = Settings::Manager::getBool("lua debug", "Lua");
        if (luaDebug)
            mCallerTraceback = state->debugTraceback();
    }

    void LuaManager::Action::safeApply(WorldView& w) const
    {
        try
        {
            apply(w);
        }
        catch (const std::exception& e)
        {
            Log(Debug::Error) << "Error in " << this->toString() << ": " << e.what();

            if (mCallerTraceback.empty())
                Log(Debug::Error) << "Set 'lua_debug=true' in settings.cfg to enable action tracebacks";
            else
                Log(Debug::Error) << "Caller " << mCallerTraceback;
        }
    }

    namespace
    {
        class FunctionAction final : public LuaManager::Action
        {
        public:
            FunctionAction(LuaUtil::LuaState* state, std::function<void()> fn, std::string_view name)
                : Action(state), mFn(std::move(fn)), mName(name) {}

            void apply(WorldView&) const override { mFn(); }
            std::string toString() const override { return "FunctionAction " + mName; }

        private:
            std::function<void()> mFn;
            std::string mName;
        };
    }

    void LuaManager::addAction(std::function<void()> action, std::string_view name)
    {
        mActionQueue.push_back(std::make_unique<FunctionAction>(&mLua, std::move(action), name));
    }

}
