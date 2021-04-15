#include "controllermanager.hpp"

#include <MyGUI_Button.h>
#include <MyGUI_InputManager.h>
#include <MyGUI_Widget.h>

#include <components/debug/debuglog.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/inputmanager.hpp"
#include "../mwbase/statemanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/player.hpp"

#include "actions.hpp"
#include "actionmanager.hpp"
#include "bindingsmanager.hpp"
#include "mousemanager.hpp"
#include "sdlmappings.hpp"

namespace MWInput
{
    ControllerManager::ControllerManager(BindingsManager* bindingsManager,
            ActionManager* actionManager,
            MouseManager* mouseManager,
            const std::string& userControllerBindingsFile,
            const std::string& controllerBindingsFile)
        : mBindingsManager(bindingsManager)
        , mActionManager(actionManager)
        , mMouseManager(mouseManager)
        , mJoystickEnabled (Settings::Manager::getBool("enable controller", "Input"))
        , mGamepadCursorSpeed(Settings::Manager::getFloat("gamepad cursor speed", "Input"))
        , mSneakToggleShortcutTimer(0.f)
        , mGamepadZoom(0)
        , mGamepadGuiCursorEnabled(true)
        , mGuiCursorEnabled(true)
        , mJoystickLastUsed(false)
        , mSneakGamepadShortcut(false)
        , mGamepadPreviewMode(false)
    {
        if (!controllerBindingsFile.empty())
        {
            SDL_GameControllerAddMappingsFromFile(controllerBindingsFile.c_str());
        }

        if (!userControllerBindingsFile.empty())
        {
            SDL_GameControllerAddMappingsFromFile(userControllerBindingsFile.c_str());
        }

        // Open all presently connected sticks
        int numSticks = SDL_NumJoysticks();
        for (int i = 0; i < numSticks; i++)
        {
            if (SDL_IsGameController(i))
            {
                SDL_ControllerDeviceEvent evt;
                evt.which = i;
                static const int fakeDeviceID = 1;
                controllerAdded(fakeDeviceID, evt);
                Log(Debug::Info) << "Detected game controller: " << SDL_GameControllerNameForIndex(i);
            }
            else
            {
                Log(Debug::Info) << "Detected unusable controller: " << SDL_JoystickNameForIndex(i);
            }
        }

        float deadZoneRadius = Settings::Manager::getFloat("joystick dead zone", "Input");
        deadZoneRadius = std::min(std::max(deadZoneRadius, 0.0f), 0.5f);
        mBindingsManager->setJoystickDeadZone(deadZoneRadius);
    }

    void ControllerManager::processChangedSettings(const Settings::CategorySettingVector& changed)
    {
        for (const auto& setting : changed)
        {
            if (setting.first == "Input" && setting.second == "enable controller")
                mJoystickEnabled = Settings::Manager::getBool("enable controller", "Input");
        }
    }

    bool ControllerManager::update(float dt)
    {
        mGamepadPreviewMode = mActionManager->isPreviewModeEnabled();

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
            float xMove = xAxis * dt * 1500.0f / uiScale;
            float yMove = yAxis * dt * 1500.0f / uiScale;

            float mouseWheelMove = -zAxis * dt * 1500.0f;
            if (xMove != 0 || yMove != 0 || mouseWheelMove != 0)
            {
                mMouseManager->injectMouseMove(xMove, yMove, mouseWheelMove);
                mMouseManager->warpMouse();
                MWBase::Environment::get().getWindowManager()->setCursorActive(true);
            }
        }

        // Disable movement in Gui mode
        if (MWBase::Environment::get().getWindowManager()->isGuiMode()
            || MWBase::Environment::get().getStateManager()->getState() != MWBase::StateManager::State_Running)
        {
            mGamepadZoom = 0;
            return false;
        }

        MWWorld::Player& player = MWBase::Environment::get().getWorld()->getPlayer();
        bool triedToMove = false;

        // Configure player movement according to controller input. Actual movement will
        // be done in the physics system.
        if (MWBase::Environment::get().getInputManager()->getControlSwitch("playercontrols"))
        {
            float xAxis = mBindingsManager->getActionValue(A_MoveLeftRight);
            float yAxis = mBindingsManager->getActionValue(A_MoveForwardBackward);
            if (xAxis != 0.5)
            {
                triedToMove = true;
                player.setLeftRight((xAxis - 0.5f) * 2);
            }

            if (yAxis != 0.5)
            {
                triedToMove = true;
                player.setAutoMove (false);
                player.setForwardBackward((0.5f - yAxis) * 2);
            }

            if (triedToMove)
            {
                mJoystickLastUsed = true;
                MWBase::Environment::get().getInputManager()->resetIdleTime();
            }

            static const bool isToggleSneak = Settings::Manager::getBool("toggle sneak", "Input");
            if (!isToggleSneak)
            {
                if (mJoystickLastUsed)
                {
                    if (mBindingsManager->actionIsActive(A_Sneak))
                    {
                        if (mSneakToggleShortcutTimer) // New Sneak Button Press
                        {
                            if (mSneakToggleShortcutTimer <= 0.3f)
                            {
                                mSneakGamepadShortcut = true;
                                mActionManager->toggleSneaking();
                            }
                            else
                                mSneakGamepadShortcut = false;
                        }

                        if (!mActionManager->isSneaking())
                            mActionManager->toggleSneaking();
                        mSneakToggleShortcutTimer = 0.f;
                    }
                    else
                    {
                        if (!mSneakGamepadShortcut && mActionManager->isSneaking())
                            mActionManager->toggleSneaking();
                        if (mSneakToggleShortcutTimer <= 0.3f)
                            mSneakToggleShortcutTimer += dt;
                    }
                }
                else
                    player.setSneak(mBindingsManager->actionIsActive(A_Sneak));
            }
        }

        if (MWBase::Environment::get().getInputManager()->getControlSwitch("playerviewswitch"))
        {
            if (!mBindingsManager->actionIsActive(A_TogglePOV))
                mGamepadZoom = 0;

            if (mGamepadZoom)
                MWBase::Environment::get().getWorld()->adjustCameraDistance(-mGamepadZoom);
        }

        return triedToMove;
    }

    void ControllerManager::buttonPressed(int deviceID, const SDL_ControllerButtonEvent &arg)
    {
        if (!mJoystickEnabled || mBindingsManager->isDetectingBindingState())
            return;

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
                        MyGUI::Button* b = MyGUI::InputManager::getInstance().getMouseFocusWidget()->castType<MyGUI::Button>(false);
                        if (b && b->getEnabled())
                            MWBase::Environment::get().getWindowManager()->playSound("Menu Click");
                    }

                    mBindingsManager->setPlayerControlsEnabled(!mousePressSuccess);
                }
            }
        }
        else
            mBindingsManager->setPlayerControlsEnabled(true);

        //esc, to leave initial movie screen
        auto kc = sdlKeyToMyGUI(SDLK_ESCAPE);
        mBindingsManager->setPlayerControlsEnabled(!MyGUI::InputManager::getInstance().injectKeyPress(kc, 0));

        if (!MWBase::Environment::get().getInputManager()->controlsDisabled())
            mBindingsManager->controllerButtonPressed(deviceID, arg);
    }

    void ControllerManager::buttonReleased(int deviceID, const SDL_ControllerButtonEvent &arg)
    {
        if (mBindingsManager->isDetectingBindingState())
        {
            mBindingsManager->controllerButtonReleased(deviceID, arg);
            return;
        }

        if (!mJoystickEnabled || MWBase::Environment::get().getInputManager()->controlsDisabled())
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
                    if (mBindingsManager->isDetectingBindingState()) // If the player just triggered binding, don't let button release bind.
                        return;

                    mBindingsManager->setPlayerControlsEnabled(!mousePressSuccess);
                }
            }
        }
        else
            mBindingsManager->setPlayerControlsEnabled(true);

        //esc, to leave initial movie screen
        auto kc = sdlKeyToMyGUI(SDLK_ESCAPE);
        mBindingsManager->setPlayerControlsEnabled(!MyGUI::InputManager::getInstance().injectKeyRelease(kc));

        mBindingsManager->controllerButtonReleased(deviceID, arg);
    }

    void ControllerManager::axisMoved(int deviceID, const SDL_ControllerAxisEvent &arg)
    {
        if (!mJoystickEnabled || MWBase::Environment::get().getInputManager()->controlsDisabled())
            return;

        mJoystickLastUsed = true;
        if (MWBase::Environment::get().getWindowManager()->isGuiMode())
        {
            gamepadToGuiControl(arg);
        }
        else
        {
            if (mGamepadPreviewMode) // Preview Mode Gamepad Zooming
            {
                if (arg.axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT)
                {
                    mGamepadZoom = arg.value * 0.85f / 1000.f / 12.f;
                    return; // Do not propagate event.
                }
                else if (arg.axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT)
                {
                    mGamepadZoom = -arg.value * 0.85f / 1000.f / 12.f;
                    return; // Do not propagate event.
                }
            }
        }
        mBindingsManager->controllerAxisMoved(deviceID, arg);
    }

    void ControllerManager::controllerAdded(int deviceID, const SDL_ControllerDeviceEvent &arg)
    {
        mBindingsManager->controllerAdded(deviceID, arg);
    }

    void ControllerManager::controllerRemoved(const SDL_ControllerDeviceEvent &arg)
    {
        mBindingsManager->controllerRemoved(arg);
    }

    bool ControllerManager::gamepadToGuiControl(const SDL_ControllerButtonEvent &arg)
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
                key = MyGUI::KeyCode::Period;
                break;
            case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
                key = MyGUI::KeyCode::Slash;
                break;
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

    bool ControllerManager::gamepadToGuiControl(const SDL_ControllerAxisEvent &arg)
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
}
