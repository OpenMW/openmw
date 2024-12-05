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
                &mTouchpadReleased, &mTouchpadMoved, &mMouseButtonPress, &mMouseButtonRelease, &mMouseWheel });
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
                case InputEvent::MouseButtonPressed:
                    mScriptsContainer->callEngineHandlers(mMouseButtonPress, std::get<int>(event.mValue));
                    break;
                case InputEvent::MouseButtonReleased:
                    mScriptsContainer->callEngineHandlers(mMouseButtonRelease, std::get<int>(event.mValue));
                    break;
                case InputEvent::MouseWheel:
                    auto wheelEvent = std::get<MWBase::LuaManager::InputEvent::WheelChange>(event.mValue);
                    mScriptsContainer->callEngineHandlers(mMouseWheel, wheelEvent.y, wheelEvent.x);
                    break;
            }
        }

    private:
        Container* mScriptsContainer;
        typename Container::EngineHandlerList mKeyPressHandlers{ "onKeyPress" };
        typename Container::EngineHandlerList mKeyReleaseHandlers{ "onKeyRelease" };
        typename Container::EngineHandlerList mControllerButtonPressHandlers{ "onControllerButtonPress" };
        typename Container::EngineHandlerList mControllerButtonReleaseHandlers{ "onControllerButtonRelease" };
        typename Container::EngineHandlerList mActionHandlers{ "onInputAction" };
        typename Container::EngineHandlerList mTouchpadPressed{ "onTouchPress" };
        typename Container::EngineHandlerList mTouchpadReleased{ "onTouchRelease" };
        typename Container::EngineHandlerList mTouchpadMoved{ "onTouchMove" };
        typename Container::EngineHandlerList mMouseButtonPress{ "onMouseButtonPress" };
        typename Container::EngineHandlerList mMouseButtonRelease{ "onMouseButtonRelease" };
        typename Container::EngineHandlerList mMouseWheel{ "onMouseWheel" };
    };
}

#endif // MWLUA_INPUTPROCESSOR_H
