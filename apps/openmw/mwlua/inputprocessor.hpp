#ifndef MWLUA_INPUTPROCESSOR_H
#define MWLUA_INPUTPROCESSOR_H

#include <SDL_events.h>

#include <components/lua/luastate.hpp>
#include <components/lua/scriptscontainer.hpp>
#include <components/sdlutil/events.hpp>

#include "../mwbase/luamanager.hpp"

namespace MWLua
{
    class InputProcessor
    {
    public:
        InputProcessor(LuaUtil::ScriptsContainer* scriptsContainer)
            : mScriptsContainer(scriptsContainer)
        {
            mScriptsContainer->registerEngineHandlers({ &mKeyPressHandlers, &mKeyReleaseHandlers,
                &mControllerButtonPressHandlers, &mControllerButtonReleaseHandlers, &mActionHandlers, &mTouchpadPressed,
                &mTouchpadReleased, &mTouchpadMoved });
        }

        void processInputEvent(const MWBase::LuaManager::InputEvent& event)
        {
            using InputEvent = MWBase::LuaManager::InputEvent;
            switch (event.mType)
            {
                case InputEvent::KeyPressed:
                    mScriptsContainer->callEngineHandlers(mKeyPressHandlers, std::get<SDL_Keysym>(event.mValue));
                    break;
                case InputEvent::KeyReleased:
                    mScriptsContainer->callEngineHandlers(mKeyReleaseHandlers, std::get<SDL_Keysym>(event.mValue));
                    break;
                case InputEvent::ControllerPressed:
                    mScriptsContainer->callEngineHandlers(mControllerButtonPressHandlers, std::get<int>(event.mValue));
                    break;
                case InputEvent::ControllerReleased:
                    mScriptsContainer->callEngineHandlers(
                        mControllerButtonReleaseHandlers, std::get<int>(event.mValue));
                    break;
                case InputEvent::Action:
                    mScriptsContainer->callEngineHandlers(mActionHandlers, std::get<int>(event.mValue));
                    break;
                case InputEvent::TouchPressed:
                    mScriptsContainer->callEngineHandlers(
                        mTouchpadPressed, std::get<SDLUtil::TouchEvent>(event.mValue));
                    break;
                case InputEvent::TouchReleased:
                    mScriptsContainer->callEngineHandlers(
                        mTouchpadReleased, std::get<SDLUtil::TouchEvent>(event.mValue));
                    break;
                case InputEvent::TouchMoved:
                    mScriptsContainer->callEngineHandlers(mTouchpadMoved, std::get<SDLUtil::TouchEvent>(event.mValue));
                    break;
            }
        }

    private:
        LuaUtil::ScriptsContainer* mScriptsContainer;
        LuaUtil::ScriptsContainer::EngineHandlerList mKeyPressHandlers{ "onKeyPress" };
        LuaUtil::ScriptsContainer::EngineHandlerList mKeyReleaseHandlers{ "onKeyRelease" };
        LuaUtil::ScriptsContainer::EngineHandlerList mControllerButtonPressHandlers{ "onControllerButtonPress" };
        LuaUtil::ScriptsContainer::EngineHandlerList mControllerButtonReleaseHandlers{ "onControllerButtonRelease" };
        LuaUtil::ScriptsContainer::EngineHandlerList mActionHandlers{ "onInputAction" };
        LuaUtil::ScriptsContainer::EngineHandlerList mTouchpadPressed{ "onTouchPress" };
        LuaUtil::ScriptsContainer::EngineHandlerList mTouchpadReleased{ "onTouchRelease" };
        LuaUtil::ScriptsContainer::EngineHandlerList mTouchpadMoved{ "onTouchMove" };
    };
}

#endif // MWLUA_INPUTPROCESSOR_H
