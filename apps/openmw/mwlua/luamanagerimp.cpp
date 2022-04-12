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

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/ptr.hpp"

#include "luabindings.hpp"
#include "userdataserializer.hpp"
#include "types/types.hpp"

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

        mL10n.init();
        std::vector<std::string> preferredLocales;
        Misc::StringUtils::split(Settings::Manager::getString("preferred locales", "General"), preferredLocales, ", ");
        mL10n.setPreferredLocales(preferredLocales);

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
        mGlobalScripts.addPackage("openmw.settings", initGlobalSettingsPackage(context));
        mGlobalScripts.addPackage("openmw.storage", initGlobalStoragePackage(context, &mGlobalStorage));

        mCameraPackage = initCameraPackage(localContext);
        mUserInterfacePackage = initUserInterfacePackage(localContext);
        mInputPackage = initInputPackage(localContext);
        mNearbyPackage = initNearbyPackage(localContext);
        mLocalSettingsPackage = initGlobalSettingsPackage(localContext);
        mPlayerSettingsPackage = initPlayerSettingsPackage(localContext);
        mLocalStoragePackage = initLocalStoragePackage(localContext, &mGlobalStorage);
        mPlayerStoragePackage = initPlayerStoragePackage(localContext, &mGlobalStorage, &mPlayerStorage);

        initConfiguration();
        mInitialized = true;
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
            c.mCallback(c.mArg);
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

        for (ObjectId id : mActorAddedEvents)
        {
            GObject obj(id, objectRegistry);
            if (obj.isValid())
                mGlobalScripts.actorActive(obj);
            else if (luaDebug)
                Log(Debug::Verbose) << "Can not call onActorActive engine handler: object" << idToString(id) << " is already removed";
        }
        mActorAddedEvents.clear();

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
        if (playerScripts && !mWorldView.isPaused())
            playerScripts->inputUpdate(MWBase::Environment::get().getFrameDuration());
        mProcessingInputEvents = false;

        MWBase::WindowManager* windowManager = MWBase::Environment::get().getWindowManager();
        for (const std::string& message : mUIMessages)
            windowManager->messageBox(message);
        mUIMessages.clear();

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
        mActiveLocalScripts.clear();
        mLocalEvents.clear();
        mGlobalEvents.clear();
        mInputEvents.clear();
        mActorAddedEvents.clear();
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
        mGlobalStorage.clearTemporary();
        mPlayerStorage.clearTemporary();
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
            localScripts = createLocalScripts(ptr, ESM::LuaScriptCfg::sPlayer);
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
            ESM::LuaScriptCfg::Flags flag = getLuaScriptFlag(ptr);
            if (!mConfiguration.getListByFlag(flag).empty())
            {
                localScripts = createLocalScripts(ptr, flag);
                localScripts->addAutoStartedScripts();  // TODO: put to a queue and apply on next `update()`
            }
        }
        if (localScripts)
        {
            mActiveLocalScripts.insert(localScripts);
            mLocalEngineEvents.push_back({getId(ptr), LocalScripts::OnActive{}});
        }

        if (ptr.getClass().isActor() && ptr != mPlayer)
            mActorAddedEvents.push_back(getId(ptr));
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

    void LuaManager::appliedToObject(const MWWorld::Ptr& toPtr, std::string_view recordId, const MWWorld::Ptr& fromPtr)
    {
        mLocalEngineEvents.push_back({getId(toPtr), LocalScripts::OnConsume{std::string(recordId)}});
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
            localScripts = createLocalScripts(ptr, getLuaScriptFlag(ptr));
            localScripts->addAutoStartedScripts();
            if (ptr.isInCell() && MWBase::Environment::get().getWorld()->isCellActive(ptr.getCell()))
                mActiveLocalScripts.insert(localScripts);
        }
        localScripts->addCustomScript(scriptId);
    }

    LocalScripts* LuaManager::createLocalScripts(const MWWorld::Ptr& ptr, ESM::LuaScriptCfg::Flags flag)
    {
        assert(mInitialized);
        assert(flag != ESM::LuaScriptCfg::sGlobal);
        assert(ptr.getType() != ESM::REC_STAT);
        std::shared_ptr<LocalScripts> scripts;
        if (flag == ESM::LuaScriptCfg::sPlayer)
        {
            assert(ptr.getCellRef().getRefId() == "player");
            scripts = std::make_shared<PlayerScripts>(&mLua, LObject(getId(ptr), mWorldView.getObjectRegistry()));
            scripts->addPackage("openmw.ui", mUserInterfacePackage);
            scripts->addPackage("openmw.camera", mCameraPackage);
            scripts->addPackage("openmw.input", mInputPackage);
            scripts->addPackage("openmw.settings", mPlayerSettingsPackage);
            scripts->addPackage("openmw.storage", mPlayerStoragePackage);
        }
        else
        {
            scripts = std::make_shared<LocalScripts>(&mLua, LObject(getId(ptr), mWorldView.getObjectRegistry()), flag);
            scripts->addPackage("openmw.settings", mLocalSettingsPackage);
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

        mGlobalScripts.setSerializer(mGlobalLoader.get());
        mGlobalScripts.load(globalScripts);
        mGlobalScripts.setSerializer(mGlobalSerializer.get());
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
        LocalScripts* scripts = createLocalScripts(ptr, getLuaScriptFlag(ptr));

        scripts->setSerializer(mLocalLoader.get());
        scripts->load(data);
        scripts->setSerializer(mLocalSerializer.get());

        // LiveCellRef is usually copied after loading, so this Ptr will become invalid and should be deregistered.
        mWorldView.getObjectRegistry()->deregisterPtr(ptr);
    }

    void LuaManager::reloadAllScripts()
    {
        Log(Debug::Info) << "Reload Lua";

        LuaUi::clearUserInterface();
        mUiResourceManager.clear();
        mLua.dropScriptCache();
        initConfiguration();

        {  // Reload global scripts
            ESM::LuaScripts data;
            mGlobalScripts.save(data);
            mGlobalScripts.load(data);
        }

        for (const auto& [id, ptr] : mWorldView.getObjectRegistry()->mObjectMapping)
        {  // Reload local scripts
            LocalScripts* scripts = ptr.getRefData().getLuaScripts();
            if (scripts == nullptr)
                continue;
            ESM::LuaScripts data;
            scripts->save(data);
            scripts->load(data);
        }
        for (LocalScripts* scripts : mActiveLocalScripts)
            scripts->receiveEngineEvent(LocalScripts::OnActive());
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
