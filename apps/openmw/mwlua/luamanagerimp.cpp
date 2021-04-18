#include "luamanagerimp.hpp"

#include <components/debug/debuglog.hpp>

#include <components/esm/esmreader.hpp>
#include <components/esm/esmwriter.hpp>
#include <components/esm/luascripts.hpp>

#include <components/lua/utilpackage.hpp>

#include "../mwbase/windowmanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/ptr.hpp"

#include "luabindings.hpp"
#include "userdataserializer.hpp"

namespace MWLua
{

    LuaManager::LuaManager(const VFS::Manager* vfs, const std::vector<std::string>& globalScriptLists) : mLua(vfs)
    {
        Log(Debug::Info) << "Lua version: " << LuaUtil::getLuaVersion();

        mGlobalSerializer = createUserdataSerializer(false, mWorldView.getObjectRegistry());
        mLocalSerializer = createUserdataSerializer(true, mWorldView.getObjectRegistry());
        mGlobalLoader = createUserdataSerializer(false, mWorldView.getObjectRegistry(), &mContentFileMapping);
        mLocalLoader = createUserdataSerializer(true, mWorldView.getObjectRegistry(), &mContentFileMapping);

        mGlobalScripts.setSerializer(mGlobalSerializer.get());

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

        mLua.addCommonPackage("openmw.async", getAsyncPackageInitializer(context));
        mLua.addCommonPackage("openmw.util", LuaUtil::initUtilPackage(mLua.sol()));
        mLua.addCommonPackage("openmw.core", initCorePackage(context));
        mLua.addCommonPackage("openmw.query", initQueryPackage(context));
        mGlobalScripts.addPackage("openmw.world", initWorldPackage(context));
        mCameraPackage = initCameraPackage(localContext);
        mUserInterfacePackage = initUserInterfacePackage(localContext);
        mNearbyPackage = initNearbyPackage(localContext);

        auto endsWith = [](std::string_view s, std::string_view suffix)
        {
            return s.size() >= suffix.size() && std::equal(suffix.rbegin(), suffix.rend(), s.rbegin());
        };
        for (const std::string& scriptListFile : globalScriptLists)
        {
            if (!endsWith(scriptListFile, ".omwscripts"))
            {
                Log(Debug::Error) << "Script list should have suffix '.omwscripts', got: '" << scriptListFile << "'";
                continue;
            }
            std::string content(std::istreambuf_iterator<char>(*vfs->get(scriptListFile)), {});
            std::string_view view(content);
            while (!view.empty())
            {
                size_t pos = 0;
                while (pos < view.size() && view[pos] != '\n')
                    pos++;
                std::string_view line = view.substr(0, pos);
                view = view.substr(pos + 1);
                if (line.empty() || line[0] == '#')
                    continue;
                if (line.back() == '\r')
                    line = line.substr(0, pos - 1);
                if (endsWith(line, ".lua"))
                    mGlobalScriptList.push_back(std::string(line));
                else
                    Log(Debug::Error) << "Lua script should have suffix '.lua', got: '" << line.substr(0, 300) << "'";
            }
        }
    }

    void LuaManager::init()
    {
        mKeyPressEvents.clear();
        for (const std::string& path : mGlobalScriptList)
            if (mGlobalScripts.addNewScript(path))
                Log(Debug::Info) << "Global script started: " << path;
    }

    void LuaManager::update(bool paused, float dt)
    {
        if (!mPlayer.isEmpty())
        {
            MWWorld::Ptr newPlayerPtr = MWBase::Environment::get().getWorld()->getPlayerPtr();
            if (!(getId(mPlayer) == getId(newPlayerPtr)))
                throw std::logic_error("Player Refnum was changed unexpectedly");
            if (!mPlayer.isInCell() || !newPlayerPtr.isInCell() || mPlayer.getCell() != newPlayerPtr.getCell())
            {
                mPlayer = newPlayerPtr;
                mWorldView.getObjectRegistry()->registerPtr(mPlayer);
            }
        }
        mWorldView.update();

        if (paused)
        {
            mKeyPressEvents.clear();
            return;
        }

        std::vector<GlobalEvent> globalEvents = std::move(mGlobalEvents);
        std::vector<LocalEvent> localEvents = std::move(mLocalEvents);
        mGlobalEvents = std::vector<GlobalEvent>();
        mLocalEvents = std::vector<LocalEvent>();

        {  // Update time and process timers
            double seconds = mWorldView.getGameTimeInSeconds() + dt;
            mWorldView.setGameTimeInSeconds(seconds);
            double hours = mWorldView.getGameTimeInHours();

            mGlobalScripts.processTimers(seconds, hours);
            for (LocalScripts* scripts : mActiveLocalScripts)
                scripts->processTimers(seconds, hours);
        }

        for (GlobalEvent& e : globalEvents)
            mGlobalScripts.receiveEvent(e.eventName, e.eventData);
        for (LocalEvent& e : localEvents)
        {
            LObject obj(e.dest, mWorldView.getObjectRegistry());
            LocalScripts* scripts = obj.isValid() ? obj.ptr().getRefData().getLuaScripts() : nullptr;
            if (scripts)
                scripts->receiveEvent(e.eventName, e.eventData);
            else
                Log(Debug::Debug) << "Ignored event " << e.eventName << " to L" << idToString(e.dest)
                                  << ". Object not found or has no attached scripts";
        }

        if (mPlayerChanged)
        {
            mPlayerChanged = false;
            mGlobalScripts.playerAdded(GObject(getId(mPlayer), mWorldView.getObjectRegistry()));
        }

        if (mPlayerScripts)
        {
            for (const SDL_Keysym key : mKeyPressEvents)
                mPlayerScripts->keyPress(key.sym, key.mod);
        }
        mKeyPressEvents.clear();

        for (LocalScripts* localScripts : mObjectInactiveEvents)
            localScripts->becomeInactive();
        for (LocalScripts* localScripts : mObjectActiveEvents)
            localScripts->becomeActive();
        mObjectActiveEvents.clear();
        mObjectInactiveEvents.clear();

        for (ObjectId id : mActorAddedEvents)
            mGlobalScripts.actorActive(GObject(id, mWorldView.getObjectRegistry()));
        mActorAddedEvents.clear();

        mGlobalScripts.update(dt);
        for (LocalScripts* scripts : mActiveLocalScripts)
            scripts->update(dt);
    }

    void LuaManager::applyQueuedChanges()
    {
        MWBase::WindowManager* windowManager = MWBase::Environment::get().getWindowManager();
        for (const std::string& message : mUIMessages)
            windowManager->messageBox(message);
        mUIMessages.clear();

        for (std::unique_ptr<Action>& action : mActionQueue)
            action->apply(mWorldView);
        mActionQueue.clear();
        
        if (mTeleportPlayerAction)
            mTeleportPlayerAction->apply(mWorldView);
        mTeleportPlayerAction.reset();
    }

    void LuaManager::clear()
    {
        mActiveLocalScripts.clear();
        mLocalEvents.clear();
        mGlobalEvents.clear();
        mKeyPressEvents.clear();
        mActorAddedEvents.clear();
        mObjectActiveEvents.clear();
        mObjectInactiveEvents.clear();
        mPlayerChanged = false;
        mPlayerScripts = nullptr;
        mWorldView.clear();
        if (!mPlayer.isEmpty())
        {
            mPlayer.getCellRef().unsetRefNum();
            mPlayer.getRefData().setLuaScripts(nullptr);
            mPlayer = MWWorld::Ptr();
        }
    }

    void LuaManager::objectAddedToScene(const MWWorld::Ptr& ptr)
    {
        mWorldView.objectAddedToScene(ptr);  // assigns generated RefNum if it is not set yet.

        LocalScripts* localScripts = ptr.getRefData().getLuaScripts();
        if (localScripts)
        {
            mActiveLocalScripts.insert(localScripts);
            mObjectActiveEvents.push_back(localScripts);
        }

        if (ptr.getClass().isActor() && ptr != mPlayer)
            mActorAddedEvents.push_back(getId(ptr));
    }

    void LuaManager::setupPlayer(const MWWorld::Ptr& ptr)
    {
        if (!mPlayer.isEmpty())
            throw std::logic_error("Player is initialized twice");
        mWorldView.objectAddedToScene(ptr);
        mPlayer = ptr;
        MWWorld::RefData& refData = ptr.getRefData();
        if (!refData.getLuaScripts())
            createLocalScripts(ptr);
        if (!mPlayerScripts)
            throw std::logic_error("mPlayerScripts not initialized");
        mActiveLocalScripts.insert(mPlayerScripts);
        mPlayerChanged = true;
    }

    void LuaManager::objectRemovedFromScene(const MWWorld::Ptr& ptr)
    {
        mWorldView.objectRemovedFromScene(ptr);
        LocalScripts* localScripts = ptr.getRefData().getLuaScripts();
        if (localScripts)
        {
            mActiveLocalScripts.erase(localScripts);
            mObjectInactiveEvents.push_back(localScripts);
        }

        // TODO: call mWorldView.objectUnloaded if object is unloaded from memory (does it ever happen?) and ptr becomes invalid.
    }

    void LuaManager::keyPressed(const SDL_KeyboardEvent& arg)
    {
        mKeyPressEvents.push_back(arg.keysym);
    }

    MWBase::LuaManager::ActorControls* LuaManager::getActorControls(const MWWorld::Ptr& ptr) const
    {
        LocalScripts* localScripts = ptr.getRefData().getLuaScripts();
        if (!localScripts)
            return nullptr;
        return localScripts->getActorControls();
    }

    void LuaManager::addLocalScript(const MWWorld::Ptr& ptr, const std::string& scriptPath)
    {
        MWWorld::RefData& refData = ptr.getRefData();
        if (!refData.getLuaScripts())
            mActiveLocalScripts.insert(createLocalScripts(ptr));
        refData.getLuaScripts()->addNewScript(scriptPath);
    }

    LocalScripts* LuaManager::createLocalScripts(const MWWorld::Ptr& ptr)
    {
        std::unique_ptr<LocalScripts> scripts;
        // When loading a game, it can be called before LuaManager::setPlayer,
        // so we can't just check ptr == mPlayer here.
        if (*ptr.getCellRef().getRefIdPtr() == "player")
        {
            mPlayerScripts = new PlayerScripts(&mLua, LObject(getId(ptr), mWorldView.getObjectRegistry()));
            scripts = std::unique_ptr<LocalScripts>(mPlayerScripts);
            scripts->addPackage("openmw.ui", mUserInterfacePackage);
            scripts->addPackage("openmw.camera", mCameraPackage);
        }
        else
            scripts = LocalScripts::create(&mLua, LObject(getId(ptr), mWorldView.getObjectRegistry()));
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
        mGlobalScripts.load(globalScripts, false);
        mGlobalScripts.setSerializer(mGlobalSerializer.get());
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

        scripts->setSerializer(mLocalLoader.get());
        scripts->load(data, true);
        scripts->setSerializer(mLocalSerializer.get());
    }

    void LuaManager::reloadAllScripts()
    {
        Log(Debug::Info) << "Reload Lua";
        mLua.dropScriptCache();

        {  // Reload global scripts
            ESM::LuaScripts data;
            mGlobalScripts.save(data);
            mGlobalScripts.removeAllScripts();
            for (const std::string& path : mGlobalScriptList)
                if (mGlobalScripts.addNewScript(path))
                    Log(Debug::Info) << "Global script restarted: " << path;
            mGlobalScripts.load(data, false);
        }

        for (const auto& [id, ptr] : mWorldView.getObjectRegistry()->mObjectMapping)
        {  // Reload local scripts
            LocalScripts* scripts = ptr.getRefData().getLuaScripts();
            if (scripts == nullptr)
                continue;
            ESM::LuaScripts data;
            scripts->save(data);
            scripts->load(data, true);
        }
    }

}
