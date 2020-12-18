#ifndef GAME_MWBASE_LUAMANAGER_H
#define GAME_MWBASE_LUAMANAGER_H

#include <SDL_events.h>

namespace MWWorld
{
    class Ptr;
}

namespace MWBase
{

    class LuaManager
    {
    public:
        virtual ~LuaManager() = default;

        virtual void newGameStarted() = 0;
        virtual void objectAddedToScene(const MWWorld::Ptr& ptr) = 0;
        virtual void objectRemovedFromScene(const MWWorld::Ptr& ptr) = 0;
        virtual void keyPressed(const SDL_KeyboardEvent &arg) = 0;

        struct ActorControls {
            bool controlledFromLua;

            bool jump;
            bool run;
            float movement;
            float sideMovement;
            float turn;
        };

        virtual const ActorControls* getActorControls(const MWWorld::Ptr&) const = 0;

        virtual void clear() = 0;
        virtual void setupPlayer(const MWWorld::Ptr&) = 0;
    };

}

#endif  // GAME_MWBASE_LUAMANAGER_H
