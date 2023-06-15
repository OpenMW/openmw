#ifndef GAME_MWBASE_LUAMANAGER_H
#define GAME_MWBASE_LUAMANAGER_H

#include <map>
#include <string>
#include <variant>

#include <SDL_events.h>

#include <components/sdlutil/events.hpp>

namespace MWWorld
{
    class CellStore;
    class Ptr;
}

namespace Loading
{
    class Listener;
}

namespace ESM
{
    class ESMReader;
    class ESMWriter;
    struct LuaScripts;
}

namespace MWBase
{
    // \brief LuaManager is the central interface through which the engine invokes lua scripts.
    //
    // The native side invokes functions on this interface, which queues events to be handled by the
    // scripts in the lua thread. Synchronous calls are not possible.
    //
    // The main implementation is in apps/openmw/mwlua/luamanagerimp.cpp.
    // Lua logic in general lives under apps/openmw/mwlua and this interface is
    // the main way for the rest of the engine to interact with the logic there.
    class LuaManager
    {
    public:
        virtual ~LuaManager() = default;

        virtual void newGameStarted() = 0;
        virtual void gameLoaded() = 0;
        virtual void objectAddedToScene(const MWWorld::Ptr& ptr) = 0;
        virtual void objectRemovedFromScene(const MWWorld::Ptr& ptr) = 0;
        virtual void itemConsumed(const MWWorld::Ptr& consumable, const MWWorld::Ptr& actor) = 0;
        virtual void objectActivated(const MWWorld::Ptr& object, const MWWorld::Ptr& actor) = 0;
        virtual void exteriorCreated(MWWorld::CellStore& cell) = 0;
        // TODO: notify LuaManager about other events
        // virtual void objectOnHit(const MWWorld::Ptr &ptr, float damage, bool ishealth, const MWWorld::Ptr &object,
        //                          const MWWorld::Ptr &attacker, const osg::Vec3f &hitPosition, bool successful) = 0;

        struct InputEvent
        {
            enum
            {
                KeyPressed,
                KeyReleased,
                ControllerPressed,
                ControllerReleased,
                Action,
                TouchPressed,
                TouchReleased,
                TouchMoved,
            } mType;
            std::variant<SDL_Keysym, int, SDLUtil::TouchEvent> mValue;
        };
        virtual void inputEvent(const InputEvent& event) = 0;

        struct ActorControls
        {
            bool mDisableAI = false;
            bool mChanged = false;

            bool mJump = false;
            bool mRun = false;
            bool mSneak = false;
            float mMovement = 0;
            float mSideMovement = 0;
            float mPitchChange = 0;
            float mYawChange = 0;
            int mUse = 0;
        };

        virtual ActorControls* getActorControls(const MWWorld::Ptr&) const = 0;

        virtual void clear() = 0;
        virtual void setupPlayer(const MWWorld::Ptr&) = 0;

        // Saving
        int countSavedGameRecords() const { return 1; }
        virtual void write(ESM::ESMWriter& writer, Loading::Listener& progress) = 0;
        virtual void saveLocalScripts(const MWWorld::Ptr& ptr, ESM::LuaScripts& data) = 0;

        // Loading from a save
        virtual void readRecord(ESM::ESMReader& reader, uint32_t type) = 0;
        virtual void loadLocalScripts(const MWWorld::Ptr& ptr, const ESM::LuaScripts& data) = 0;

        // Should be called before loading. The map is used to fix refnums if the order of content files was changed.
        virtual void setContentFileMapping(const std::map<int, int>&) = 0;

        // Drops script cache and reloads all scripts. Calls `onSave` and `onLoad` for every script.
        virtual void reloadAllScripts() = 0;

        virtual void handleConsoleCommand(
            const std::string& consoleMode, const std::string& command, const MWWorld::Ptr& selectedPtr)
            = 0;

        virtual std::string formatResourceUsageStats() const = 0;
    };

}

#endif // GAME_MWBASE_LUAMANAGER_H
