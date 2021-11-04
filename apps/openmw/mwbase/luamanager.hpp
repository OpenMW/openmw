#ifndef GAME_MWBASE_LUAMANAGER_H
#define GAME_MWBASE_LUAMANAGER_H

#include <variant>
#include <SDL_events.h>

namespace MWWorld
{
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

    class LuaManager
    {
    public:
        virtual ~LuaManager() = default;

        virtual void newGameStarted() = 0;
        virtual void gameLoaded() = 0;
        virtual void registerObject(const MWWorld::Ptr& ptr) = 0;
        virtual void deregisterObject(const MWWorld::Ptr& ptr) = 0;
        virtual void objectAddedToScene(const MWWorld::Ptr& ptr) = 0;
        virtual void objectRemovedFromScene(const MWWorld::Ptr& ptr) = 0;
        virtual void appliedToObject(const MWWorld::Ptr& toPtr, std::string_view recordId, const MWWorld::Ptr& fromPtr) = 0;
        // TODO: notify LuaManager about other events
        // virtual void objectOnHit(const MWWorld::Ptr &ptr, float damage, bool ishealth, const MWWorld::Ptr &object,
        //                          const MWWorld::Ptr &attacker, const osg::Vec3f &hitPosition, bool successful) = 0;

        struct InputEvent
        {
            enum {KeyPressed, KeyReleased, ControllerPressed, ControllerReleased, Action} mType;
            std::variant<SDL_Keysym, int> mValue;
        };
        virtual void inputEvent(const InputEvent& event) = 0;

        struct ActorControls
        {
            bool mDisableAI = false;
            bool mChanged = false;

            bool mJump = false;
            bool mRun = false;
            float mMovement = 0;
            float mSideMovement = 0;
            float mTurn = 0;
        };

        virtual ActorControls* getActorControls(const MWWorld::Ptr&) const = 0;

        virtual void clear() = 0;
        virtual void setupPlayer(const MWWorld::Ptr&) = 0;

        // Saving
        int countSavedGameRecords() const { return 1; };
        virtual void write(ESM::ESMWriter& writer, Loading::Listener& progress) = 0;
        virtual void saveLocalScripts(const MWWorld::Ptr& ptr, ESM::LuaScripts& data) = 0;

        // Loading from a save
        virtual void readRecord(ESM::ESMReader& reader, uint32_t type) = 0;
        virtual void loadLocalScripts(const MWWorld::Ptr& ptr, const ESM::LuaScripts& data) = 0;

        // Should be called before loading. The map is used to fix refnums if the order of content files was changed.
        virtual void setContentFileMapping(const std::map<int, int>&) = 0;

        // Drops script cache and reloads all scripts. Calls `onSave` and `onLoad` for every script.
        virtual void reloadAllScripts() = 0;
    };

}

#endif  // GAME_MWBASE_LUAMANAGER_H
