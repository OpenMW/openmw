#ifndef MWLUA_PLAYERSCRIPTS_H
#define MWLUA_PLAYERSCRIPTS_H

#include <SDL_events.h>

#include <components/sdlutil/events.hpp>

#include "../mwbase/luamanager.hpp"

#include "localscripts.hpp"

namespace MWLua
{

    class PlayerScripts : public LocalScripts
    {
    public:
        PlayerScripts(LuaUtil::LuaState* lua, const LObject& obj) : LocalScripts(lua, obj, ESM::LuaScriptCfg::sPlayer)
        {
            registerEngineHandlers({
                &mKeyPressHandlers, &mKeyReleaseHandlers,
                &mControllerButtonPressHandlers, &mControllerButtonReleaseHandlers,
                &mActionHandlers, &mInputUpdateHandlers,
                &mTouchpadPressed, &mTouchpadReleased, &mTouchpadMoved
            });
        }

        void processInputEvent(const MWBase::LuaManager::InputEvent& event)
        {
            using InputEvent = MWBase::LuaManager::InputEvent;
            switch (event.mType)
            {
            case InputEvent::KeyPressed:
                callEngineHandlers(mKeyPressHandlers, std::get<SDL_Keysym>(event.mValue));
                break;
            case InputEvent::KeyReleased:
                callEngineHandlers(mKeyReleaseHandlers, std::get<SDL_Keysym>(event.mValue));
                break;
            case InputEvent::ControllerPressed:
                callEngineHandlers(mControllerButtonPressHandlers, std::get<int>(event.mValue));
                break;
            case InputEvent::ControllerReleased:
                callEngineHandlers(mControllerButtonReleaseHandlers, std::get<int>(event.mValue));
                break;
            case InputEvent::Action:
                callEngineHandlers(mActionHandlers, std::get<int>(event.mValue));
                break;
            case InputEvent::TouchPressed:
                callEngineHandlers(mTouchpadPressed, std::get<SDLUtil::TouchEvent>(event.mValue));
                break;
            case InputEvent::TouchReleased:
                callEngineHandlers(mTouchpadReleased, std::get<SDLUtil::TouchEvent>(event.mValue));
                break;
            case InputEvent::TouchMoved:
                callEngineHandlers(mTouchpadMoved, std::get<SDLUtil::TouchEvent>(event.mValue));
                break;
            }
        }

        void inputUpdate(float dt) { callEngineHandlers(mInputUpdateHandlers, dt); }

    private:
        EngineHandlerList mKeyPressHandlers{"onKeyPress"};
        EngineHandlerList mKeyReleaseHandlers{"onKeyRelease"};
        EngineHandlerList mControllerButtonPressHandlers{"onControllerButtonPress"};
        EngineHandlerList mControllerButtonReleaseHandlers{"onControllerButtonRelease"};
        EngineHandlerList mActionHandlers{"onInputAction"};
        EngineHandlerList mInputUpdateHandlers{"onInputUpdate"};
        EngineHandlerList mTouchpadPressed{ "onTouchPress" };
        EngineHandlerList mTouchpadReleased{ "onTouchRelease" };
        EngineHandlerList mTouchpadMoved{ "onTouchMove" };
    };

}

#endif // MWLUA_PLAYERSCRIPTS_H
