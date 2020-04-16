#include "controllermanager.hpp"

#include <MyGUI_Button.h>
#include <MyGUI_InputManager.h>
#include <MyGUI_Widget.h>

#include <extern/oics/ICSInputControlSystem.h>

#include <components/debug/debuglog.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/inputmanager.hpp"
#include "../mwbase/statemanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/player.hpp"

#include "actions.hpp"
#include "actionmanager.hpp"
#include "mousemanager.hpp"

namespace MWInput
{
    static const int sFakeDeviceID = 1;

    ControllerManager::ControllerManager(ICS::InputControlSystem* inputBinder,
            SDLUtil::InputWrapper* inputWrapper,
            ActionManager* actionManager,
            MouseManager* mouseManager,
            const std::string& userControllerBindingsFile,
            const std::string& controllerBindingsFile)
        : mInputBinder(inputBinder)
        , mInputWrapper(inputWrapper)
        , mActionManager(actionManager)
        , mMouseManager(mouseManager)
        , mJoystickEnabled (Settings::Manager::getBool("enable controller", "Input"))
        , mGamepadCursorSpeed(Settings::Manager::getFloat("gamepad cursor speed", "Input"))
        , mInvUiScalingFactor(1.f)
        , mSneakToggleShortcutTimer(0.f)
        , mGamepadZoom(0)
        , mGamepadGuiCursorEnabled(true)
        , mControlsDisabled(false)
        , mJoystickLastUsed(false)
        , mSneakGamepadShortcut(false)
        , mGamepadPreviewMode(false)
    {
        if(!controllerBindingsFile.empty())
        {
            SDL_GameControllerAddMappingsFromFile(controllerBindingsFile.c_str());
        }
        if(!userControllerBindingsFile.empty())
        {
            SDL_GameControllerAddMappingsFromFile(userControllerBindingsFile.c_str());
        }

        // Open all presently connected sticks
        int numSticks = SDL_NumJoysticks();
        for(int i = 0; i < numSticks; i++)
        {
            if(SDL_IsGameController(i))
            {
                SDL_ControllerDeviceEvent evt;
                evt.which = i;
                controllerAdded(sFakeDeviceID, evt);
                Log(Debug::Info) << "Detected game controller: " << SDL_GameControllerNameForIndex(i);
            }
            else
            {
                Log(Debug::Info) << "Detected unusable controller: " << SDL_JoystickNameForIndex(i);
            }
        }

        float uiScale = Settings::Manager::getFloat("scaling factor", "GUI");
        if (uiScale != 0.f)
            mInvUiScalingFactor = 1.f / uiScale;
    }

    void ControllerManager::clear()
    {
    }

    void ControllerManager::processChangedSettings(const Settings::CategorySettingVector& changed)
    {
        for (Settings::CategorySettingVector::const_iterator it = changed.begin(); it != changed.end(); ++it)
        {
            if (it->first == "Input" && it->second == "enable controller")
                mJoystickEnabled = Settings::Manager::getBool("enable controller", "Input");
        }
    }

    bool ControllerManager::actionIsActive (int id)
    {
        return (mInputBinder->getChannel (id)->getValue ()==1.0);
    }

    bool ControllerManager::update(float dt, bool disableControls, bool gamepadPreviewMode)
    {
        mControlsDisabled = disableControls;
        mGamepadPreviewMode = gamepadPreviewMode;

        if (mGuiCursorEnabled && !(mJoystickLastUsed && !mGamepadGuiCursorEnabled))
        {
            float xAxis = mInputBinder->getChannel(A_MoveLeftRight)->getValue()*2.0f-1.0f;
            float yAxis = mInputBinder->getChannel(A_MoveForwardBackward)->getValue()*2.0f-1.0f;
            float zAxis = mInputBinder->getChannel(A_LookUpDown)->getValue()*2.0f-1.0f;

            xAxis *= (1.5f - mInputBinder->getChannel(A_Use)->getValue());
            yAxis *= (1.5f - mInputBinder->getChannel(A_Use)->getValue());

            // We keep track of our own mouse position, so that moving the mouse while in
            // game mode does not move the position of the GUI cursor
            float xMove = xAxis * dt * 1500.0f * mInvUiScalingFactor * mGamepadCursorSpeed;
            float yMove = yAxis * dt * 1500.0f * mInvUiScalingFactor * mGamepadCursorSpeed;
            if (xMove != 0 || yMove != 0 || zAxis != 0)
            {
                int mouseWheelMove = static_cast<int>(-zAxis * dt * 1500.0f);

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
            float xAxis = mInputBinder->getChannel(A_MoveLeftRight)->getValue();
            float yAxis = mInputBinder->getChannel(A_MoveForwardBackward)->getValue();
            if (xAxis != .5)
            {
                triedToMove = true;
                player.setLeftRight((xAxis - 0.5f) * 2);
            }

            if (yAxis != .5)
            {
                triedToMove = true;
                player.setAutoMove (false);
                player.setForwardBackward((yAxis - 0.5f) * 2 * -1);
            }

            if (triedToMove)
                mJoystickLastUsed = true;

            if (triedToMove)
                MWBase::Environment::get().getInputManager()->resetIdleTime();

            static const bool isToggleSneak = Settings::Manager::getBool("toggle sneak", "Input");
            if (!isToggleSneak)
            {
                if(mJoystickLastUsed)
                {
                    if(actionIsActive(A_Sneak))
                    {
                        if(mSneakToggleShortcutTimer) // New Sneak Button Press
                        {
                            if(mSneakToggleShortcutTimer <= 0.3f)
                            {
                                mSneakGamepadShortcut = true;
                                mActionManager->toggleSneaking();
                            }
                            else
                                mSneakGamepadShortcut = false;
                        }

                        if(!mActionManager->isSneaking())
                            mActionManager->toggleSneaking();
                        mSneakToggleShortcutTimer = 0.f;
                    }
                    else
                    {
                        if(!mSneakGamepadShortcut && mActionManager->isSneaking())
                            mActionManager->toggleSneaking();
                        if(mSneakToggleShortcutTimer <= 0.3f)
                            mSneakToggleShortcutTimer += dt;
                    }
                }
                else
                    player.setSneak(actionIsActive(A_Sneak));
            }
        }

        if (MWBase::Environment::get().getInputManager()->getControlSwitch("playerviewswitch"))
        {
            if (!actionIsActive(A_TogglePOV))
                mGamepadZoom = 0;

            if(mGamepadZoom)
            {
                MWBase::Environment::get().getWorld()->changeVanityModeScale(mGamepadZoom);
                MWBase::Environment::get().getWorld()->setCameraDistance(mGamepadZoom, true, true);
            }
        }

        return triedToMove;
    }

    void ControllerManager::buttonPressed(int deviceID, const SDL_ControllerButtonEvent &arg )
    {
        if (!mJoystickEnabled || mInputBinder->detectingBindingState())
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

                    MWBase::Environment::get().getInputManager()->setPlayerControlsEnabled(!mousePressSuccess);
                }
            }
        }
        else
            MWBase::Environment::get().getInputManager()->setPlayerControlsEnabled(true);

        //esc, to leave initial movie screen
        auto kc = mInputWrapper->sdl2OISKeyCode(SDLK_ESCAPE);
        MWBase::Environment::get().getInputManager()->setPlayerControlsEnabled(!MyGUI::InputManager::getInstance().injectKeyPress(MyGUI::KeyCode::Enum(kc), 0));

        if (!mControlsDisabled)
            mInputBinder->buttonPressed(deviceID, arg);
    }

    void ControllerManager::buttonReleased(int deviceID, const SDL_ControllerButtonEvent &arg )
    {
        if(mInputBinder->detectingBindingState())
        {
            mInputBinder->buttonReleased(deviceID, arg);
            return;
        }
        if (!mJoystickEnabled || mControlsDisabled)
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
                    if (mInputBinder->detectingBindingState()) // If the player just triggered binding, don't let button release bind.
                        return;

                    MWBase::Environment::get().getInputManager()->setPlayerControlsEnabled(!mousePressSuccess);
                }
            }
        }
        else
            MWBase::Environment::get().getInputManager()->setPlayerControlsEnabled(true);

        //esc, to leave initial movie screen
        auto kc = mInputWrapper->sdl2OISKeyCode(SDLK_ESCAPE);
        MWBase::Environment::get().getInputManager()->setPlayerControlsEnabled(!MyGUI::InputManager::getInstance().injectKeyRelease(MyGUI::KeyCode::Enum(kc)));

        mInputBinder->buttonReleased(deviceID, arg);
    }

    void ControllerManager::axisMoved(int deviceID, const SDL_ControllerAxisEvent &arg)
    {
        if(!mJoystickEnabled || mControlsDisabled)
            return;

        mJoystickLastUsed = true;
        if (MWBase::Environment::get().getWindowManager()->isGuiMode())
        {
            gamepadToGuiControl(arg);
        }
        else
        {
            if(mGamepadPreviewMode && arg.value) // Preview Mode Gamepad Zooming
            {
                if(arg.axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT)
                {
                    mGamepadZoom = arg.value * 0.85f / 1000.f;
                    return; // Do not propagate event.
                }
                else if(arg.axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT)
                {
                    mGamepadZoom = -arg.value * 0.85f / 1000.f;
                    return; // Do not propagate event.
                }
            }
        }
        mInputBinder->axisMoved(deviceID, arg);
    }

    void ControllerManager::controllerAdded(int deviceID, const SDL_ControllerDeviceEvent &arg)
    {
        mInputBinder->controllerAdded(deviceID, arg);
    }

    void ControllerManager::controllerRemoved(const SDL_ControllerDeviceEvent &arg)
    {
        mInputBinder->controllerRemoved(arg);
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
