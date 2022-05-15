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
                &mConsoleCommandHandlers, &mKeyPressHandlers, &mKeyReleaseHandlers,
                &mControllerButtonPressHandlers, &mControllerButtonReleaseHandlers,
                &mActionHandlers, &mOnFrameHandlers,
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

        void onFrame(float dt) { callEngineHandlers(mOnFrameHandlers, dt); }

        bool consoleCommand(const std::string& consoleMode, const std::string& command, const sol::object& selectedObject)
        {
            callEngineHandlers(mConsoleCommandHandlers, consoleMode, command, selectedObject);
            return !mConsoleCommandHandlers.mList.empty();
        }

    private:
        EngineHandlerList mConsoleCommandHandlers{"onConsoleCommand"};
        EngineHandlerList mKeyPressHandlers{"onKeyPress"};
        EngineHandlerList mKeyReleaseHandlers{"onKeyRelease"};
        EngineHandlerList mControllerButtonPressHandlers{"onControllerButtonPress"};
        EngineHandlerList mControllerButtonReleaseHandlers{"onControllerButtonRelease"};
        EngineHandlerList mActionHandlers{"onInputAction"};
        EngineHandlerList mOnFrameHandlers{"onFrame"};
        EngineHandlerList mTouchpadPressed{ "onTouchPress" };
        EngineHandlerList mTouchpadReleased{ "onTouchRelease" };
        EngineHandlerList mTouchpadMoved{ "onTouchMove" };
    };

}

#endif // MWLUA_PLAYERSCRIPTS_H
