#include "luamanagerimp.hpp"

#include <components/debug/debuglog.hpp>
#include <components/lua/utilpackage.hpp>

#include "../mwworld/class.hpp"
#include "../mwworld/ptr.hpp"

#include "luabindings.hpp"
#include "userdataserializer.hpp"

namespace MWLua
{

    LuaManager::LuaManager(const VFS::Manager* vfs) : mLua(vfs)
    {
        Log(Debug::Info) << "Lua version: " << LuaUtil::getLuaVersion();
        mGlobalSerializer = createUserdataSerializer(false, mWorldView.getObjectRegistry());
        mLocalSerializer = createUserdataSerializer(true, mWorldView.getObjectRegistry());
        mGlobalScripts.setSerializer(mGlobalSerializer.get());

        Context context;
        context.mLuaManager = this;
        context.mLua = &mLua;
        context.mWorldView = &mWorldView;
        context.mLocalEventQueue = &mLocalEvents;
        context.mGlobalEventQueue = &mGlobalEvents;
        context.mSerializer = mGlobalSerializer.get();

        Context localContext = context;
        localContext.mSerializer = mLocalSerializer.get();

        initObjectBindingsForGlobalScripts(context);
        initObjectBindingsForLocalScripts(localContext);
        LocalScripts::initializeSelfPackage(localContext);

        mLua.addCommonPackage("openmw.util", LuaUtil::initUtilPackage(mLua.sol()));
        mLua.addCommonPackage("openmw.core", initCorePackage(context));
        mGlobalScripts.addPackage("openmw.world", initWorldPackage(context));
        mNearbyPackage = initNearbyPackage(localContext);
    }

    void LuaManager::init()
    {
        mKeyPressEvents.clear();
        if (mGlobalScripts.addNewScript("test.lua"))
            Log(Debug::Info) << "Global script started: test.lua";
    }

    void LuaManager::update(bool paused, float dt)
    {
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

        for (ObjectId id : mActorAddedEvents)
            mGlobalScripts.actorActive(GObject(id, mWorldView.getObjectRegistry()));
        mActorAddedEvents.clear();

        mGlobalScripts.update(dt);
        for (LocalScripts* scripts : mActiveLocalScripts)
            scripts->update(dt);
    }

    void LuaManager::applyQueuedChanges()
    {
    }

    void LuaManager::clear()
    {
        mActiveLocalScripts.clear();
        mLocalEvents.clear();
        mGlobalEvents.clear();
        mKeyPressEvents.clear();
        mActorAddedEvents.clear();
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
            mActiveLocalScripts.insert(localScripts);

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
            mActiveLocalScripts.erase(localScripts);

        // TODO: call mWorldView.objectUnloaded if object is unloaded from memory (does it ever happen?) and ptr becomes invalid.
    }

    void LuaManager::keyPressed(const SDL_KeyboardEvent& arg)
    {
        mKeyPressEvents.push_back(arg.keysym);
    }
    
    const MWBase::LuaManager::ActorControls* LuaManager::getActorControls(const MWWorld::Ptr& ptr) const
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
            // TODO: scripts->addPackage("openmw.ui", ...);
            // TODO: scripts->addPackage("openmw.camera", ...);
        }
        else
            scripts = LocalScripts::create(&mLua, LObject(getId(ptr), mWorldView.getObjectRegistry()));
        scripts->addPackage("openmw.nearby", mNearbyPackage);
        scripts->setSerializer(mLocalSerializer.get());

        MWWorld::RefData& refData = ptr.getRefData();
        refData.setLuaScripts(std::move(scripts));
        return refData.getLuaScripts();
    }

}
