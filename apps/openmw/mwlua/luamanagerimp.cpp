#include "luamanagerimp.hpp"

#include <filesystem>

#include <osg/Stats>

#include "sol/state_view.hpp"

#include <components/debug/debuglog.hpp>

#include <components/esm/luascripts.hpp>
#include <components/esm3/esmreader.hpp>
#include <components/esm3/esmwriter.hpp>

#include <components/settings/values.hpp>

#include <components/l10n/manager.hpp>

#include <components/lua_ui/content.hpp>
#include <components/lua_ui/util.hpp>

#include "../mwbase/windowmanager.hpp"

#include "../mwrender/postprocessor.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/ptr.hpp"
#include "../mwworld/scene.hpp"

#include "luabindings.hpp"
#include "playerscripts.hpp"
#include "types/types.hpp"
#include "userdataserializer.hpp"

namespace MWLua
{

    static LuaUtil::LuaStateSettings createLuaStateSettings()
    {
        if (!Settings::lua().mLuaProfiler)
            LuaUtil::LuaState::disableProfiler();
        return { .mInstructionLimit = Settings::lua().mInstructionLimitPerCall,
            .mMemoryLimit = Settings::lua().mMemoryLimit,
            .mSmallAllocMaxSize = Settings::lua().mSmallAllocMaxSize,
            .mLogMemoryUsage = Settings::lua().mLogMemoryUsage };
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
        mConfiguration.init(MWBase::Environment::get().getESMStore()->getLuaScriptsCfg());
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
        context.mLuaEvents = &mLuaEvents;
        context.mSerializer = mGlobalSerializer.get();

        Context localContext = context;
        localContext.mIsGlobal = false;
        localContext.mSerializer = mLocalSerializer.get();

        for (const auto& [name, package] : initCommonPackages(context))
            mLua.addCommonPackage(name, package);
        for (const auto& [name, package] : initGlobalPackages(context))
            mGlobalScripts.addPackage(name, package);

        mLocalPackages = initLocalPackages(localContext);
        mPlayerPackages = initPlayerPackages(localContext);
        mPlayerPackages.insert(mLocalPackages.begin(), mLocalPackages.end());

        LuaUtil::LuaStorage::initLuaBindings(mLua.sol());
        mGlobalScripts.addPackage(
            "openmw.storage", LuaUtil::LuaStorage::initGlobalPackage(mLua.sol(), &mGlobalStorage));
        mLocalPackages["openmw.storage"] = LuaUtil::LuaStorage::initLocalPackage(mLua.sol(), &mGlobalStorage);
        mPlayerPackages["openmw.storage"]
            = LuaUtil::LuaStorage::initPlayerPackage(mLua.sol(), &mGlobalStorage, &mPlayerStorage);

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
        if (Settings::lua().mGcStepsPerFrame > 0)
            lua_gc(mLua.sol(), LUA_GCSTEP, Settings::lua().mGcStepsPerFrame);

        if (mPlayer.isEmpty())
            return; // The game is not started yet.

        float frameDuration = MWBase::Environment::get().getFrameDuration();

        MWWorld::Ptr newPlayerPtr = MWBase::Environment::get().getWorld()->getPlayerPtr();
        if (!(getId(mPlayer) == getId(newPlayerPtr)))
            throw std::logic_error("Player RefNum was changed unexpectedly");
        if (!mPlayer.isInCell() || !newPlayerPtr.isInCell() || mPlayer.getCell() != newPlayerPtr.getCell())
        {
            mPlayer = newPlayerPtr; // player was moved to another cell, update ptr in registry
            MWBase::Environment::get().getWorldModel()->registerPtr(mPlayer);
        }

        mWorldView.update();

        std::erase_if(mActiveLocalScripts,
            [](const LocalScripts* l) { return l->getPtr().isEmpty() || l->getPtr().getRefData().isDeleted(); });

        mGlobalScripts.statsNextFrame();
        for (LocalScripts* scripts : mActiveLocalScripts)
            scripts->statsNextFrame();

        mLuaEvents.finalizeEventBatch();

        if (!mWorldView.isPaused())
        { // Update time and process timers
            double simulationTime = mWorldView.getSimulationTime() + frameDuration;
            mWorldView.setSimulationTime(simulationTime);
            double gameTime = mWorldView.getGameTime();

            mGlobalScripts.processTimers(simulationTime, gameTime);
            for (LocalScripts* scripts : mActiveLocalScripts)
                scripts->processTimers(simulationTime, gameTime);
        }

        // Run event handlers for events that were sent before `finalizeEventBatch`.
        mLuaEvents.callEventHandlers();

        // Run queued callbacks
        for (CallbackWithData& c : mQueuedCallbacks)
            c.mCallback.tryCall(c.mArg);
        mQueuedCallbacks.clear();

        // Run engine handlers
        mEngineEvents.callEngineHandlers();
        if (!mWorldView.isPaused())
        {
            for (LocalScripts* scripts : mActiveLocalScripts)
                scripts->update(frameDuration);
            mGlobalScripts.update(frameDuration);
        }
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

        for (DelayedAction& action : mActionQueue)
            action.apply();
        mActionQueue.clear();

        if (mTeleportPlayerAction)
            mTeleportPlayerAction->apply();
        mTeleportPlayerAction.reset();
    }

    void LuaManager::clear()
    {
        LuaUi::clearUserInterface();
        mUiResourceManager.clear();
        MWBase::Environment::get().getWindowManager()->setConsoleMode("");
        MWBase::Environment::get().getWorld()->getPostProcessor()->disableDynamicShaders();
        mActiveLocalScripts.clear();
        mLuaEvents.clear();
        mEngineEvents.clear();
        mInputEvents.clear();
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
        for (int i = 0; i < 5; ++i)
            lua_gc(mLua.sol(), LUA_GCCOLLECT, 0);
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
        mEngineEvents.addToQueue(EngineEvents::OnActive{ getId(ptr) });
    }

    void LuaManager::newGameStarted()
    {
        mInputEvents.clear();
        mGlobalScripts.addAutoStartedScripts();
        mGlobalScriptsStarted = true;
        mEngineEvents.addToQueue(EngineEvents::OnNewGame{});
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
        mEngineEvents.addToQueue(EngineEvents::OnActive{ getId(ptr) });

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
            mActiveLocalScripts.insert(localScripts);
    }

    void LuaManager::objectRemovedFromScene(const MWWorld::Ptr& ptr)
    {
        mWorldView.objectRemovedFromScene(ptr);
        LocalScripts* localScripts = ptr.getRefData().getLuaScripts();
        if (localScripts)
        {
            mActiveLocalScripts.erase(localScripts);
            if (!MWBase::Environment::get().getWorldModel()->getPtr(getId(ptr)).isEmpty())
                mEngineEvents.addToQueue(EngineEvents::OnInactive{ getId(ptr) });
        }
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
            for (const auto& [name, package] : mPlayerPackages)
                scripts->addPackage(name, package);
        }
        else
        {
            scripts = std::make_shared<LocalScripts>(&mLua, LObject(getId(ptr)));
            if (!autoStartConf.has_value())
                autoStartConf = mConfiguration.getLocalConf(type, ptr.getCellRef().getRefId(), getId(ptr));
            scripts->setAutoStartConf(std::move(*autoStartConf));
            for (const auto& [name, package] : mLocalPackages)
                scripts->addPackage(name, package);
        }
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
        mLuaEvents.save(writer);

        writer.endRecord(ESM::REC_LUAM);
    }

    void LuaManager::readRecord(ESM::ESMReader& reader, uint32_t type)
    {
        if (type != ESM::REC_LUAM)
            throw std::runtime_error("ESM::REC_LUAM is expected");

        mWorldView.load(reader);
        ESM::LuaScripts globalScripts;
        globalScripts.load(reader);
        mLuaEvents.load(mLua.sol(), reader, mContentFileMapping, mGlobalLoader.get());

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
            scripts->setActive(true);
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

    LuaManager::DelayedAction::DelayedAction(LuaUtil::LuaState* state, std::function<void()> fn, std::string_view name)
        : mFn(std::move(fn))
        , mName(name)
    {
        if (Settings::lua().mLuaDebug)
            mCallerTraceback = state->debugTraceback();
    }

    void LuaManager::DelayedAction::apply() const
    {
        try
        {
            mFn();
        }
        catch (const std::exception& e)
        {
            Log(Debug::Error) << "Error in DelayedAction " << mName << ": " << e.what();

            if (mCallerTraceback.empty())
                Log(Debug::Error) << "Set 'lua_debug=true' in settings.cfg to enable action tracebacks";
            else
                Log(Debug::Error) << "Caller " << mCallerTraceback;
        }
    }

    void LuaManager::addAction(std::function<void()> action, std::string_view name)
    {
        mActionQueue.emplace_back(&mLua, std::move(action), name);
    }

    void LuaManager::addTeleportPlayerAction(std::function<void()> action)
    {
        mTeleportPlayerAction = DelayedAction(&mLua, std::move(action), "TeleportPlayer");
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

        const uint64_t smallAllocSize = Settings::lua().mSmallAllocMaxSize;
        out << "Total memory usage:";
        outMemSize(mLua.getTotalMemoryUsage());
        out << "\n";
        out << "LuaUtil::ScriptsContainer count: " << LuaUtil::ScriptsContainer::getInstanceCount() << "\n";
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
            out << "Profiled object (selected in the in-game console): " << selectedPtr.toString() << "\n";
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
