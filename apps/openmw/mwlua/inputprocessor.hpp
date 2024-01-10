#ifndef MWLUA_INPUTPROCESSOR_H
#define MWLUA_INPUTPROCESSOR_H

#include <SDL_events.h>

#include <components/sdlutil/events.hpp>

#include "../mwbase/luamanager.hpp"

namespace MWLua
{
    template <class Container>
    class InputProcessor
    {
    public:
        InputProcessor(Container* scriptsContainer)
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
        Container* mScriptsContainer;
        Container::EngineHandlerList mKeyPressHandlers{ "onKeyPress" };
        Container::EngineHandlerList mKeyReleaseHandlers{ "onKeyRelease" };
        Container::EngineHandlerList mControllerButtonPressHandlers{ "onControllerButtonPress" };
        Container::EngineHandlerList mControllerButtonReleaseHandlers{ "onControllerButtonRelease" };
        Container::EngineHandlerList mActionHandlers{ "onInputAction" };
        Container::EngineHandlerList mTouchpadPressed{ "onTouchPress" };
        Container::EngineHandlerList mTouchpadReleased{ "onTouchRelease" };
        Container::EngineHandlerList mTouchpadMoved{ "onTouchMove" };
    };
}

#endif // MWLUA_INPUTPROCESSOR_H
