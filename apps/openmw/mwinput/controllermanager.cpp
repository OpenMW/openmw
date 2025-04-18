#include "controllermanager.hpp"

#include <MyGUI_Button.h>
#include <MyGUI_InputManager.h>

#include <SDL.h>

#include <components/debug/debuglog.hpp>
#include <components/esm/refid.hpp>
#include <components/files/conversion.hpp>
#include <components/sdlutil/sdlmappings.hpp>
#include <components/settings/values.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/inputmanager.hpp"
#include "../mwbase/luamanager.hpp"
#include "../mwbase/statemanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "actions.hpp"
#include "bindingsmanager.hpp"
#include "mousemanager.hpp"

namespace MWInput
{
    ControllerManager::ControllerManager(BindingsManager* bindingsManager, MouseManager* mouseManager,
        const std::filesystem::path& userControllerBindingsFile, const std::filesystem::path& controllerBindingsFile)
        : mBindingsManager(bindingsManager)
        , mMouseManager(mouseManager)
        , mGyroAvailable(false)
        , mGamepadGuiCursorEnabled(true)
        , mGuiCursorEnabled(true)
        , mJoystickLastUsed(false)
    {
        if (!controllerBindingsFile.empty())
        {
            const int result
                = SDL_GameControllerAddMappingsFromFile(Files::pathToUnicodeString(controllerBindingsFile).c_str());
            if (result < 0)
                Log(Debug::Error) << "Failed to add game controller mappings from file \"" << controllerBindingsFile
                                  << "\": " << SDL_GetError();
        }

        if (!userControllerBindingsFile.empty())
        {
            const int result
                = SDL_GameControllerAddMappingsFromFile(Files::pathToUnicodeString(userControllerBindingsFile).c_str());
            if (result < 0)
                Log(Debug::Error) << "Failed to add game controller mappings from user file \""
                                  << userControllerBindingsFile << "\": " << SDL_GetError();
        }

        // Open all presently connected sticks
        const int numSticks = SDL_NumJoysticks();
        if (numSticks < 0)
            Log(Debug::Error) << "Failed to get number of joysticks: " << SDL_GetError();

        for (int i = 0; i < numSticks; i++)
        {
            if (SDL_IsGameController(i))
            {
                SDL_ControllerDeviceEvent evt;
                evt.which = i;
                static const int fakeDeviceID = 1;
                ControllerManager::controllerAdded(fakeDeviceID, evt);
                if (const char* name = SDL_GameControllerNameForIndex(i))
                    Log(Debug::Info) << "Detected game controller: " << name;
                else
                    Log(Debug::Warning) << "Detected game controller without a name: " << SDL_GetError();
            }
            else
            {
                if (const char* name = SDL_JoystickNameForIndex(i))
                    Log(Debug::Info) << "Detected unusable controller: " << name;
                else
                    Log(Debug::Warning) << "Detected unusable controller without a name: " << SDL_GetError();
            }
        }

        mBindingsManager->setJoystickDeadZone(Settings::input().mJoystickDeadZone);
    }

    void ControllerManager::update(float dt)
    {
        if (mGuiCursorEnabled && !(mJoystickLastUsed && !mGamepadGuiCursorEnabled))
        {
            float xAxis = mBindingsManager->getActionValue(A_MoveLeftRight) * 2.0f - 1.0f;
            float yAxis = mBindingsManager->getActionValue(A_MoveForwardBackward) * 2.0f - 1.0f;
            float zAxis = mBindingsManager->getActionValue(A_LookUpDown) * 2.0f - 1.0f;

            xAxis *= (1.5f - mBindingsManager->getActionValue(A_Use));
            yAxis *= (1.5f - mBindingsManager->getActionValue(A_Use));

            // We keep track of our own mouse position, so that moving the mouse while in
            // game mode does not move the position of the GUI cursor
            float uiScale = MWBase::Environment::get().getWindowManager()->getScalingFactor();
            const float gamepadCursorSpeed = Settings::input().mGamepadCursorSpeed;
            const float xMove = xAxis * dt * 1500.0f / uiScale * gamepadCursorSpeed;
            const float yMove = yAxis * dt * 1500.0f / uiScale * gamepadCursorSpeed;

            float mouseWheelMove = -zAxis * dt * 1500.0f;
            if (xMove != 0 || yMove != 0 || mouseWheelMove != 0)
            {
                mMouseManager->injectMouseMove(xMove, yMove, mouseWheelMove);
                mMouseManager->warpMouse();
                MWBase::Environment::get().getWindowManager()->setCursorActive(true);
            }
        }

        if (!MWBase::Environment::get().getWindowManager()->isGuiMode()
            && MWBase::Environment::get().getStateManager()->getState() == MWBase::StateManager::State_Running
            && MWBase::Environment::get().getInputManager()->getControlSwitch("playercontrols"))
        {
            float xAxis = mBindingsManager->getActionValue(A_MoveLeftRight);
            float yAxis = mBindingsManager->getActionValue(A_MoveForwardBackward);
            if (xAxis != 0.5 || yAxis != 0.5)
            {
                mJoystickLastUsed = true;
                MWBase::Environment::get().getInputManager()->resetIdleTime();
            }
        }
    }

    void ControllerManager::buttonPressed(int deviceID, const SDL_ControllerButtonEvent& arg)
    {
        if (!Settings::input().mEnableController || mBindingsManager->isDetectingBindingState())
            return;

        MWBase::Environment::get().getLuaManager()->inputEvent(
            { MWBase::LuaManager::InputEvent::ControllerPressed, arg.button });

        mJoystickLastUsed = true;
        if (MWBase::Environment::get().getWindowManager()->isGuiMode())
        {
            if (gamepadToGuiControl(arg))
                return;

            if (mGamepadGuiCursorEnabled)
            {
                // Temporary mouse binding until keyboard controls are available:
                if (arg.button == SDL_CONTROLLER_BUTTON_A) // We'll pretend that A is left click.
                {
                    bool mousePressSuccess = mMouseManager->injectMouseButtonPress(SDL_BUTTON_LEFT);
                    if (MyGUI::InputManager::getInstance().getMouseFocusWidget())
                    {
                        MyGUI::Button* b
                            = MyGUI::InputManager::getInstance().getMouseFocusWidget()->castType<MyGUI::Button>(false);
                        if (b && b->getEnabled())
                            MWBase::Environment::get().getWindowManager()->playSound(
                                ESM::RefId::stringRefId("Menu Click"));
                    }

                    mBindingsManager->setPlayerControlsEnabled(!mousePressSuccess);
                }
            }
        }
        else
            mBindingsManager->setPlayerControlsEnabled(true);

        // esc, to leave initial movie screen
        auto kc = SDLUtil::sdlKeyToMyGUI(SDLK_ESCAPE);
        mBindingsManager->setPlayerControlsEnabled(!MyGUI::InputManager::getInstance().injectKeyPress(kc, 0));

        if (!MWBase::Environment::get().getInputManager()->controlsDisabled())
            mBindingsManager->controllerButtonPressed(deviceID, arg);
    }

    void ControllerManager::buttonReleased(int deviceID, const SDL_ControllerButtonEvent& arg)
    {
        if (mBindingsManager->isDetectingBindingState())
        {
            mBindingsManager->controllerButtonReleased(deviceID, arg);
            return;
        }

        if (Settings::input().mEnableController)
        {
            MWBase::Environment::get().getLuaManager()->inputEvent(
                { MWBase::LuaManager::InputEvent::ControllerReleased, arg.button });
        }

        if (!Settings::input().mEnableController || MWBase::Environment::get().getInputManager()->controlsDisabled())
            return;

        mJoystickLastUsed = true;
        if (MWBase::Environment::get().getWindowManager()->isGuiMode())
        {
            if (mGamepadGuiCursorEnabled)
            {
                // Temporary mouse binding until keyboard controls are available:
                if (arg.button == SDL_CONTROLLER_BUTTON_A) // We'll pretend that A is left click.
                {
                    bool mousePressSuccess = mMouseManager->injectMouseButtonRelease(SDL_BUTTON_LEFT);
                    if (mBindingsManager->isDetectingBindingState()) // If the player just triggered binding, don't let
                                                                     // button release bind.
                        return;

                    mBindingsManager->setPlayerControlsEnabled(!mousePressSuccess);
                }
            }
        }
        else
            mBindingsManager->setPlayerControlsEnabled(true);

        // esc, to leave initial movie screen
        auto kc = SDLUtil::sdlKeyToMyGUI(SDLK_ESCAPE);
        mBindingsManager->setPlayerControlsEnabled(!MyGUI::InputManager::getInstance().injectKeyRelease(kc));

        mBindingsManager->controllerButtonReleased(deviceID, arg);
    }

    void ControllerManager::axisMoved(int deviceID, const SDL_ControllerAxisEvent& arg)
    {
        if (!Settings::input().mEnableController || MWBase::Environment::get().getInputManager()->controlsDisabled())
            return;

        mJoystickLastUsed = true;
        if (MWBase::Environment::get().getWindowManager()->isGuiMode())
        {
            gamepadToGuiControl(arg);
        }
        else if (mBindingsManager->actionIsActive(A_TogglePOV)
            && (arg.axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT || arg.axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT))
        {
            // Preview Mode Gamepad Zooming; do not propagate to mBindingsManager
            return;
        }
        mBindingsManager->controllerAxisMoved(deviceID, arg);
    }

    void ControllerManager::controllerAdded(int deviceID, const SDL_ControllerDeviceEvent& arg)
    {
        mBindingsManager->controllerAdded(deviceID, arg);
        enableGyroSensor();
    }

    void ControllerManager::controllerRemoved(const SDL_ControllerDeviceEvent& arg)
    {
        mBindingsManager->controllerRemoved(arg);
    }

    bool ControllerManager::gamepadToGuiControl(const SDL_ControllerButtonEvent& arg)
    {
        // Presumption of GUI mode will be removed in the future.
        // MyGUI KeyCodes *may* change.
        MyGUI::KeyCode key = MyGUI::KeyCode::None;
        switch (arg.button)
        {
            case SDL_CONTROLLER_BUTTON_DPAD_UP:
                key = MyGUI::KeyCode::ArrowUp;
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                key = MyGUI::KeyCode::ArrowRight;
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                key = MyGUI::KeyCode::ArrowDown;
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                key = MyGUI::KeyCode::ArrowLeft;
                break;
            case SDL_CONTROLLER_BUTTON_A:
                // If we are using the joystick as a GUI mouse, A must be handled via mouse.
                if (mGamepadGuiCursorEnabled)
                    return false;
                key = MyGUI::KeyCode::Space;
                break;
            case SDL_CONTROLLER_BUTTON_B:
                if (MyGUI::InputManager::getInstance().isModalAny())
                    MWBase::Environment::get().getWindowManager()->exitCurrentModal();
                else
                    MWBase::Environment::get().getWindowManager()->exitCurrentGuiMode();
                return true;
            case SDL_CONTROLLER_BUTTON_X:
                key = MyGUI::KeyCode::Semicolon;
                break;
            case SDL_CONTROLLER_BUTTON_Y:
                key = MyGUI::KeyCode::Apostrophe;
                break;
            case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
                MyGUI::InputManager::getInstance().injectKeyPress(MyGUI::KeyCode::LeftShift);
                MWBase::Environment::get().getWindowManager()->injectKeyPress(MyGUI::KeyCode::Tab, 0, false);
                MyGUI::InputManager::getInstance().injectKeyRelease(MyGUI::KeyCode::LeftShift);
                return true;
            case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
                MWBase::Environment::get().getWindowManager()->injectKeyPress(MyGUI::KeyCode::Tab, 0, false);
                return true;
            case SDL_CONTROLLER_BUTTON_LEFTSTICK:
                mGamepadGuiCursorEnabled = !mGamepadGuiCursorEnabled;
                MWBase::Environment::get().getWindowManager()->setCursorActive(mGamepadGuiCursorEnabled);
                return true;
            default:
                return false;
        }

        // Some keys will work even when Text Input windows/modals are in focus.
        if (SDL_IsTextInputActive())
            return false;

        MWBase::Environment::get().getWindowManager()->injectKeyPress(key, 0, false);
        return true;
    }

    bool ControllerManager::gamepadToGuiControl(const SDL_ControllerAxisEvent& arg)
    {
        switch (arg.axis)
        {
            case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
                if (arg.value == 32767) // Treat like a button.
                    MWBase::Environment::get().getWindowManager()->injectKeyPress(MyGUI::KeyCode::Minus, 0, false);
                break;
            case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
                if (arg.value == 32767) // Treat like a button.
                    MWBase::Environment::get().getWindowManager()->injectKeyPress(MyGUI::KeyCode::Equals, 0, false);
                break;
            case SDL_CONTROLLER_AXIS_LEFTX:
            case SDL_CONTROLLER_AXIS_LEFTY:
            case SDL_CONTROLLER_AXIS_RIGHTX:
            case SDL_CONTROLLER_AXIS_RIGHTY:
                // If we are using the joystick as a GUI mouse, process mouse movement elsewhere.
                if (mGamepadGuiCursorEnabled)
                    return false;
                break;
            default:
                return false;
        }

        return true;
    }

    float ControllerManager::getAxisValue(SDL_GameControllerAxis axis) const
    {
        SDL_GameController* cntrl = mBindingsManager->getControllerOrNull();
        constexpr float axisMaxAbsoluteValue = 32768;
        if (cntrl != nullptr)
            return SDL_GameControllerGetAxis(cntrl, axis) / axisMaxAbsoluteValue;
        return 0;
    }

    bool ControllerManager::isButtonPressed(SDL_GameControllerButton button) const
    {
        SDL_GameController* cntrl = mBindingsManager->getControllerOrNull();
        if (cntrl)
            return SDL_GameControllerGetButton(cntrl, button) > 0;
        else
            return false;
    }

    void ControllerManager::enableGyroSensor()
    {
        mGyroAvailable = false;
        SDL_GameController* cntrl = mBindingsManager->getControllerOrNull();
        if (!cntrl)
            return;
        if (!SDL_GameControllerHasSensor(cntrl, SDL_SENSOR_GYRO))
            return;
        if (const int result = SDL_GameControllerSetSensorEnabled(cntrl, SDL_SENSOR_GYRO, SDL_TRUE); result < 0)
        {
            Log(Debug::Error) << "Failed to enable game controller sensor: " << SDL_GetError();
            return;
        }
        mGyroAvailable = true;
    }

    bool ControllerManager::isGyroAvailable() const
    {
        return mGyroAvailable;
    }

    std::array<float, 3> ControllerManager::getGyroValues() const
    {
        float gyro[3] = { 0.f };
        SDL_GameController* cntrl = mBindingsManager->getControllerOrNull();
        if (cntrl && mGyroAvailable)
        {
            const int result = SDL_GameControllerGetSensorData(cntrl, SDL_SENSOR_GYRO, gyro, 3);
            if (result < 0)
                Log(Debug::Error) << "Failed to get game controller sensor data: " << SDL_GetError();
        }
        return std::array<float, 3>({ gyro[0], gyro[1], gyro[2] });
    }

    void ControllerManager::touchpadMoved(int deviceId, const SDLUtil::TouchEvent& arg)
    {
        MWBase::Environment::get().getLuaManager()->inputEvent({ MWBase::LuaManager::InputEvent::TouchMoved, arg });
    }

    void ControllerManager::touchpadPressed(int deviceId, const SDLUtil::TouchEvent& arg)
    {
        MWBase::Environment::get().getLuaManager()->inputEvent({ MWBase::LuaManager::InputEvent::TouchPressed, arg });
    }

    void ControllerManager::touchpadReleased(int deviceId, const SDLUtil::TouchEvent& arg)
    {
        MWBase::Environment::get().getLuaManager()->inputEvent({ MWBase::LuaManager::InputEvent::TouchReleased, arg });
    }
}
