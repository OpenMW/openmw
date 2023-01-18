#include "luamanagerimp.hpp"

#include <filesystem>

#include <osg/Stats>

#include "sol/state_view.hpp"

#include <components/debug/debuglog.hpp>

#include <components/esm/luascripts.hpp>
#include <components/esm3/esmreader.hpp>
#include <components/esm3/esmwriter.hpp>

#include <components/settings/settings.hpp>

#include <components/l10n/manager.hpp>

#include <components/lua/utilpackage.hpp>

#include <components/lua_ui/content.hpp>
#include <components/lua_ui/util.hpp>

#include "../mwbase/windowmanager.hpp"

#include "../mwrender/postprocessor.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/ptr.hpp"
#include "../mwworld/scene.hpp"

#include "debugbindings.hpp"
#include "luabindings.hpp"
#include "playerscripts.hpp"
#include "types/types.hpp"
#include "userdataserializer.hpp"

namespace MWLua
{

    static LuaUtil::LuaStateSettings createLuaStateSettings()
    {
        if (!Settings::Manager::getBool("lua profiler", "Lua"))
            LuaUtil::LuaState::disableProfiler();
        return { .mInstructionLimit = Settings::Manager::getUInt64("instruction limit per call", "Lua"),
            .mMemoryLimit = Settings::Manager::getUInt64("memory limit", "Lua"),
            .mSmallAllocMaxSize = Settings::Manager::getUInt64("small alloc max size", "Lua"),
            .mLogMemoryUsage = Settings::Manager::getBool("log memory usage", "Lua") };
    }

    LuaManager::LuaManager(const VFS::Manager* vfs, const std::filesystem::path& libsDir)
        : mLua(vfs, &mConfiguration, createLuaStateSettings())
        , mUiResourceManager(vfs)
    {
        Log(Debug::Info) << "Lua version: " << LuaUtil::getLuaVersion();
        mLua.addInternalLibSearchPath(libsDir);

        mGlobalSerializer = createUserdataSerializer(false);
        mLocalSerializer = createUserdataSerializer(true);
        mGlobalLoader = createUserdataSerializer(false, &mContentFileMapping);
        mLocalLoader = createUserdataSerializer(true, &mContentFileMapping);

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

    void LuaManager::init()
    {
        Context context;
        context.mIsGlobal = true;
        context.mLuaManager = this;
        context.mLua = &mLua;
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

    void LuaManager::loadPermanentStorage(const std::filesystem::path& userConfigPath)
    {
        const auto globalPath = userConfigPath / "global_storage.bin";
        const auto playerPath = userConfigPath / "player_storage.bin";
        if (std::filesystem::exists(globalPath))
            mGlobalStorage.load(globalPath);
        if (std::filesystem::exists(playerPath))
            mPlayerStorage.load(playerPath);
    }

    void LuaManager::savePermanentStorage(const std::filesystem::path& userConfigPath)
    {
        mGlobalStorage.save(userConfigPath / "global_storage.bin");
        mPlayerStorage.save(userConfigPath / "player_storage.bin");
    }

    void LuaManager::update()
    {
        static const bool luaDebug = Settings::Manager::getBool("lua debug", "Lua");
        static const int gcStepCount = Settings::Manager::getInt("gc steps per frame", "Lua");
        if (gcStepCount > 0)
            lua_gc(mLua.sol(), LUA_GCSTEP, gcStepCount);

        if (mPlayer.isEmpty())
            return; // The game is not started yet.

        float frameDuration = MWBase::Environment::get().getFrameDuration();

        MWWorld::Ptr newPlayerPtr = MWBase::Environment::get().getWorld()->getPlayerPtr();
        if (!(getId(mPlayer) == getId(newPlayerPtr)))
            throw std::logic_error("Player Refnum was changed unexpectedly");
        if (!mPlayer.isInCell() || !newPlayerPtr.isInCell() || mPlayer.getCell() != newPlayerPtr.getCell())
        {
            mPlayer = newPlayerPtr; // player was moved to another cell, update ptr in registry
            MWBase::Environment::get().getWorldModel()->registerPtr(mPlayer);
        }

        mWorldView.update();

        mGlobalScripts.statsNextFrame();
        for (LocalScripts* scripts : mActiveLocalScripts)
            scripts->statsNextFrame();

        std::vector<GlobalEvent> globalEvents = std::move(mGlobalEvents);
        std::vector<LocalEvent> localEvents = std::move(mLocalEvents);
        mGlobalEvents = std::vector<GlobalEvent>();
        mLocalEvents = std::vector<LocalEvent>();

        if (!mWorldView.isPaused())
        { // Update time and process timers
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
            LObject obj(e.mDest);
            LocalScripts* scripts = obj.isValid() ? obj.ptr().getRefData().getLuaScripts() : nullptr;
            if (scripts)
                scripts->receiveEvent(e.mEventName, e.mEventData);
            else
                Log(Debug::Debug) << "Ignored event " << e.mEventName << " to L" << idToString(e.mDest)
                                  << ". Object not found or has no attached scripts";
        }

        // Run queued callbacks
        for (CallbackWithData& c : mQueuedCallbacks)
            c.mCallback.tryCall(c.mArg);
        mQueuedCallbacks.clear();

        // Engine handlers in local scripts
        for (const LocalEngineEvent& e : mLocalEngineEvents)
        {
            LObject obj(e.mDest);
            if (!obj.isValid())
            {
                if (luaDebug)
                    Log(Debug::Verbose) << "Can not call engine handlers: object" << idToString(e.mDest)
                                        << " is not found";
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
            mGlobalScripts.playerAdded(GObject(getId(mPlayer)));
        }
        if (mNewGameStarted)
        {
            mNewGameStarted = false;
            mGlobalScripts.newGameStarted();
        }

        for (ObjectId id : mObjectAddedEvents)
        {
            GObject obj(id);
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
            return; // The game is not started yet.

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
            action->safeApply();
        mActionQueue.clear();

        if (mTeleportPlayerAction)
            mTeleportPlayerAction->safeApply();
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
        mLocalEngineEvents.push_back({ getId(ptr), LocalScripts::OnActive{} });
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
        mWorldView.objectAddedToScene(ptr); // assigns generated RefNum if it is not set yet.

        LocalScripts* localScripts = ptr.getRefData().getLuaScripts();
        if (!localScripts)
        {
            LuaUtil::ScriptIdsWithInitializationData autoStartConf
                = mConfiguration.getLocalConf(getLiveCellRefType(ptr.mRef), ptr.getCellRef().getRefId(), getId(ptr));
            if (!autoStartConf.empty())
            {
                localScripts = createLocalScripts(ptr, std::move(autoStartConf));
                localScripts->addAutoStartedScripts(); // TODO: put to a queue and apply on next `update()`
            }
        }
        if (localScripts)
        {
            mActiveLocalScripts.insert(localScripts);
            mLocalEngineEvents.push_back({ getId(ptr), LocalScripts::OnActive{} });
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
            if (!MWBase::Environment::get().getWorldModel()->getPtr(getId(ptr)).isEmpty())
                mLocalEngineEvents.push_back({ getId(ptr), LocalScripts::OnInactive{} });
        }
    }

    void LuaManager::itemConsumed(const MWWorld::Ptr& consumable, const MWWorld::Ptr& actor)
    {
        MWBase::Environment::get().getWorldModel()->registerPtr(consumable);
        mLocalEngineEvents.push_back({ getId(actor), LocalScripts::OnConsume{ LObject(getId(consumable)) } });
    }

    void LuaManager::objectActivated(const MWWorld::Ptr& object, const MWWorld::Ptr& actor)
    {
        mLocalEngineEvents.push_back({ getId(object), LocalScripts::OnActivated{ LObject(getId(actor)) } });
    }

    MWBase::LuaManager::ActorControls* LuaManager::getActorControls(const MWWorld::Ptr& ptr) const
    {
        LocalScripts* localScripts = ptr.getRefData().getLuaScripts();
        if (!localScripts)
            return nullptr;
        return localScripts->getActorControls();
    }

    void LuaManager::addCustomLocalScript(const MWWorld::Ptr& ptr, int scriptId, std::string_view initData)
    {
        LocalScripts* localScripts = ptr.getRefData().getLuaScripts();
        if (!localScripts)
        {
            localScripts = createLocalScripts(ptr);
            localScripts->addAutoStartedScripts();
            if (ptr.isInCell() && MWBase::Environment::get().getWorldScene()->isCellActive(*ptr.getCell()))
                mActiveLocalScripts.insert(localScripts);
        }
        localScripts->addCustomScript(scriptId, initData);
    }

    LocalScripts* LuaManager::createLocalScripts(
        const MWWorld::Ptr& ptr, std::optional<LuaUtil::ScriptIdsWithInitializationData> autoStartConf)
    {
        assert(mInitialized);
        std::shared_ptr<LocalScripts> scripts;
        const uint32_t type = getLiveCellRefType(ptr.mRef);
        if (type == ESM::REC_STAT)
            throw std::runtime_error("Lua scripts on static objects are not allowed");
        else if (type == ESM::REC_INTERNAL_PLAYER)
        {
            scripts = std::make_shared<PlayerScripts>(&mLua, LObject(getId(ptr)));
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
            scripts = std::make_shared<LocalScripts>(&mLua, LObject(getId(ptr)));
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

        MWBase::Environment::get().getWorldModel()->registerPtr(ptr);
        LocalScripts* scripts = createLocalScripts(ptr);

        scripts->setSerializer(mLocalSerializer.get());
        scripts->setSavedDataDeserializer(mLocalLoader.get());
        scripts->load(data);

        // LiveCellRef is usually copied after loading, so this Ptr will become invalid and should be deregistered.
        MWBase::Environment::get().getWorldModel()->deregisterPtr(ptr);
    }

    void LuaManager::reloadAllScripts()
    {
        Log(Debug::Info) << "Reload Lua";

        LuaUi::clearUserInterface();
        MWBase::Environment::get().getWindowManager()->setConsoleMode("");
        MWBase::Environment::get().getL10nManager()->dropCache();
        mUiResourceManager.clear();
        mLua.dropScriptCache();
        initConfiguration();

        { // Reload global scripts
            mGlobalScripts.setSavedDataDeserializer(mGlobalSerializer.get());
            ESM::LuaScripts data;
            mGlobalScripts.save(data);
            mGlobalScripts.load(data);
        }

        for (const auto& [id, ptr] : MWBase::Environment::get().getWorldModel()->getAllPtrs())
        { // Reload local scripts
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

    void LuaManager::handleConsoleCommand(
        const std::string& consoleMode, const std::string& command, const MWWorld::Ptr& selectedPtr)
    {
        PlayerScripts* playerScripts = nullptr;
        if (!mPlayer.isEmpty())
            playerScripts = dynamic_cast<PlayerScripts*>(mPlayer.getRefData().getLuaScripts());
        if (!playerScripts)
        {
            MWBase::Environment::get().getWindowManager()->printToConsole(
                "You must enter a game session to run Lua commands\n", MWBase::WindowManager::sConsoleColor_Error);
            return;
        }
        sol::object selected = sol::nil;
        if (!selectedPtr.isEmpty())
            selected = sol::make_object(mLua.sol(), LObject(getId(selectedPtr)));
        if (!playerScripts->consoleCommand(consoleMode, command, selected))
            MWBase::Environment::get().getWindowManager()->printToConsole(
                "No Lua handlers for console\n", MWBase::WindowManager::sConsoleColor_Error);
    }

    LuaManager::Action::Action(LuaUtil::LuaState* state)
    {
        static const bool luaDebug = Settings::Manager::getBool("lua debug", "Lua");
        if (luaDebug)
            mCallerTraceback = state->debugTraceback();
    }

    void LuaManager::Action::safeApply() const
    {
        try
        {
            apply();
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
                : Action(state)
                , mFn(std::move(fn))
                , mName(name)
            {
            }

            void apply() const override { mFn(); }
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

    void LuaManager::reportStats(unsigned int frameNumber, osg::Stats& stats) const
    {
        stats.setAttribute(frameNumber, "Lua UsedMemory", mLua.getTotalMemoryUsage());
    }

    std::string LuaManager::formatResourceUsageStats() const
    {
        if (!LuaUtil::LuaState::isProfilerEnabled())
            return "Lua profiler is disabled";

        std::stringstream out;

        constexpr int nameW = 50;
        constexpr int valueW = 12;

        auto outMemSize = [&](int64_t bytes) {
            constexpr int64_t limit = 10000;
            out << std::right << std::setw(valueW - 3);
            if (bytes < limit)
                out << bytes << " B ";
            else if (bytes < limit * 1024)
                out << (bytes / 1024) << " KB";
            else if (bytes < limit * 1024 * 1024)
                out << (bytes / (1024 * 1024)) << " MB";
            else
                out << (bytes / (1024 * 1024 * 1024)) << " GB";
        };

        static const uint64_t smallAllocSize = Settings::Manager::getUInt64("small alloc max size", "Lua");
        out << "Total memory usage:";
        outMemSize(mLua.getTotalMemoryUsage());
        out << "\n";
        out << "LuaUtil::ScriptsContainer count: " << LuaUtil::ScriptsContainer::getInstanceCount() << "\n";
        out << "LuaUi::Content count: " << LuaUi::Content::getInstanceCount() << "\n";
        out << "\n";
        out << "small alloc max size = " << smallAllocSize << " (section [Lua] in settings.cfg)\n";
        out << "Smaller values give more information for the profiler, but increase performance overhead.\n";
        out << "  Memory allocations <= " << smallAllocSize << " bytes:";
        outMemSize(mLua.getSmallAllocMemoryUsage());
        out << " (not tracked)\n";
        out << "  Memory allocations >  " << smallAllocSize << " bytes:";
        outMemSize(mLua.getTotalMemoryUsage() - mLua.getSmallAllocMemoryUsage());
        out << " (see the table below)\n\n";

        using Stats = LuaUtil::ScriptsContainer::ScriptStats;

        std::vector<Stats> activeStats;
        mGlobalScripts.collectStats(activeStats);
        for (LocalScripts* scripts : mActiveLocalScripts)
            scripts->collectStats(activeStats);

        std::vector<Stats> selectedStats;
        MWWorld::Ptr selectedPtr = MWBase::Environment::get().getWindowManager()->getConsoleSelectedObject();
        LocalScripts* selectedScripts = nullptr;
        if (!selectedPtr.isEmpty())
        {
            selectedScripts = selectedPtr.getRefData().getLuaScripts();
            if (selectedScripts)
                selectedScripts->collectStats(selectedStats);
            out << "Profiled object (selected in the in-game console): " << ptrToString(selectedPtr) << "\n";
        }
        else
            out << "No selected object. Use the in-game console to select an object for detailed profile.\n";
        out << "\n";

        out << "Legend\n";
        out << "  ops:        Averaged number of Lua instruction per frame;\n";
        out << "  memory:     Aggregated size of Lua allocations > " << smallAllocSize << " bytes;\n";
        out << "  [all]:      Sum over all instances of each script;\n";
        out << "  [active]:   Sum over all active (i.e. currently in scene) instances of each script;\n";
        out << "  [inactive]: Sum over all inactive instances of each script;\n";
        out << "  [for selected object]: Only for the object that is selected in the console;\n";
        out << "\n";

        out << std::left;
        out << " " << std::setw(nameW + 2) << "*** Resource usage per script";
        out << std::right;
        out << std::setw(valueW) << "ops";
        out << std::setw(valueW) << "memory";
        out << std::setw(valueW) << "memory";
        out << std::setw(valueW) << "ops";
        out << std::setw(valueW) << "memory";
        out << "\n";
        out << std::left << " " << std::setw(nameW + 2) << "[name]" << std::right;
        out << std::setw(valueW) << "[all]";
        out << std::setw(valueW) << "[active]";
        out << std::setw(valueW) << "[inactive]";
        out << std::setw(valueW * 2) << "[for selected object]";
        out << "\n";

        for (size_t i = 0; i < mConfiguration.size(); ++i)
        {
            bool isGlobal = mConfiguration[i].mFlags & ESM::LuaScriptCfg::sGlobal;

            out << std::left;
            out << " " << std::setw(nameW) << mConfiguration[i].mScriptPath;
            if (mConfiguration[i].mScriptPath.size() > nameW)
                out << "\n " << std::setw(nameW) << ""; // if path is too long, break line
            out << std::right;
            out << std::setw(valueW) << static_cast<int64_t>(activeStats[i].mAvgInstructionCount);
            outMemSize(activeStats[i].mMemoryUsage);
            outMemSize(mLua.getMemoryUsageByScriptIndex(i) - activeStats[i].mMemoryUsage);

            if (isGlobal)
                out << std::setw(valueW * 2) << "NA (global script)";
            else if (selectedPtr.isEmpty())
                out << std::setw(valueW * 2) << "NA (not selected) ";
            else if (!selectedScripts || !selectedScripts->hasScript(i))
            {
                out << std::setw(valueW) << "-";
                outMemSize(selectedStats[i].mMemoryUsage);
            }
            else
            {
                out << std::setw(valueW) << static_cast<int64_t>(selectedStats[i].mAvgInstructionCount);
                outMemSize(selectedStats[i].mMemoryUsage);
            }
            out << "\n";
        }

        return out.str();
    }
}
