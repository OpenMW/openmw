#include "inputmanagerimp.hpp"

#include <osgViewer/ViewerEventHandlers>

#include <MyGUI_InputManager.h>
#include <MyGUI_Widget.h>
#include <MyGUI_Button.h>
#include <MyGUI_EditBox.h>

#include <SDL_version.h>

#include <components/debug/debuglog.hpp>
#include <components/sdlutil/sdlinputwrapper.hpp>
#include <components/esm/esmwriter.hpp>
#include <components/esm/esmreader.hpp>
#include <components/esm/controlsstate.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/statemanager.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "../mwworld/player.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/actorutil.hpp"

#include "actionmanager.hpp"
#include "controllermanager.hpp"
#include "keyboardmanager.hpp"
#include "mousemanager.hpp"
#include "sdlmappings.hpp"
#include "sensormanager.hpp"

namespace MWInput
{
    InputManager::InputManager(
            SDL_Window* window,
            osg::ref_ptr<osgViewer::Viewer> viewer,
            osg::ref_ptr<osgViewer::ScreenCaptureHandler> screenCaptureHandler,
            osgViewer::ScreenCaptureHandler::CaptureOperation *screenCaptureOperation,
            const std::string& userFile, bool userFileExists, const std::string& userControllerBindingsFile,
            const std::string& controllerBindingsFile, bool grab)
        : mUserFile(userFile)
        , mDragDrop(false)
        , mGrabCursor(Settings::Manager::getBool("grab cursor", "Input"))
        , mGuiCursorEnabled(true)
        , mDetectingKeyboard(false)
        , mFakeDeviceID(1)
    {
        mInputWrapper = new SDLUtil::InputWrapper(window, viewer, grab);
        mInputWrapper->setWindowEventCallback(MWBase::Environment::get().getWindowManager());

        std::string file = userFileExists ? userFile : "";
        mInputBinder = new ICS::InputControlSystem(file, true, this, nullptr, A_Last);

        loadKeyDefaults();
        loadControllerDefaults();

        for (int i = 0; i < A_Last; ++i)
        {
            mInputBinder->getChannel (i)->addListener (this);
        }

        mControlSwitch["playercontrols"]      = true;
        mControlSwitch["playerfighting"]      = true;
        mControlSwitch["playerjumping"]       = true;
        mControlSwitch["playerlooking"]       = true;
        mControlSwitch["playermagic"]         = true;
        mControlSwitch["playerviewswitch"]    = true;
        mControlSwitch["vanitymode"]          = true;

        mActionManager = new ActionManager(mInputBinder, screenCaptureOperation, viewer, screenCaptureHandler);

        mKeyboardManager = new KeyboardManager(mInputBinder, mInputWrapper, mActionManager);
        mInputWrapper->setKeyboardEventCallback(mKeyboardManager);

        mMouseManager = new MouseManager(mInputBinder, mInputWrapper, window);
        mInputWrapper->setMouseEventCallback(mMouseManager);

        mControllerManager = new ControllerManager(mInputBinder, mInputWrapper, mActionManager, mMouseManager, userControllerBindingsFile, controllerBindingsFile);
        mInputWrapper->setControllerEventCallback(mControllerManager);

        mSensorManager = new SensorManager();
        mInputWrapper->setSensorEventCallback(mSensorManager);
    }

    void InputManager::clear()
    {
        // Enable all controls
        for (std::map<std::string, bool>::iterator it = mControlSwitch.begin(); it != mControlSwitch.end(); ++it)
            it->second = true;

        mActionManager->clear();
        mControllerManager->clear();
        mSensorManager->clear();
        mMouseManager->clear();
    }

    InputManager::~InputManager()
    {
        delete mActionManager;
        delete mControllerManager;
        delete mKeyboardManager;
        delete mMouseManager;
        delete mSensorManager;

        mInputBinder->save(mUserFile);
        delete mInputBinder;

        delete mInputWrapper;
    }

    void InputManager::setPlayerControlsEnabled(bool enabled)
    {
        int playerChannels[] = {A_AutoMove, A_AlwaysRun, A_ToggleWeapon,
                                A_ToggleSpell, A_Rest, A_QuickKey1, A_QuickKey2,
                                A_QuickKey3, A_QuickKey4, A_QuickKey5, A_QuickKey6,
                                A_QuickKey7, A_QuickKey8, A_QuickKey9, A_QuickKey10,
                                A_Use, A_Journal};

        for(size_t i = 0; i < sizeof(playerChannels)/sizeof(playerChannels[0]); i++) {
            int pc = playerChannels[i];
            mInputBinder->getChannel(pc)->setEnabled(enabled);
        }
    }

    void InputManager::channelChanged(ICS::Channel* channel, float currentValue, float previousValue)
    {
        resetIdleTime ();

        int action = channel->getNumber();

        if (mDragDrop && action != A_GameMenu && action != A_Inventory)
            return;

        if((previousValue == 1 || previousValue == 0) && (currentValue==1 || currentValue==0))
        {
            //Is a normal button press, so don't change it at all
        }
        //Otherwise only trigger button presses as they go through specific points
        else if(previousValue >= .8 && currentValue < .8)
        {
            currentValue = 0.0;
            previousValue = 1.0;
        }
        else if(previousValue <= .6 && currentValue > .6)
        {
            currentValue = 1.0;
            previousValue = 0.0;
        }
        else
        {
            //If it's not switching between those values, ignore the channel change.
            return;
        }

        if (mControlSwitch["playercontrols"])
        {
            bool joystickUsed = mControllerManager->joystickLastUsed();
            if (action == A_Use)
            {
                if(joystickUsed && currentValue == 1.0 && actionIsActive(A_ToggleWeapon))
                    action = A_CycleWeaponRight;

                else if (joystickUsed && currentValue == 1.0 && actionIsActive(A_ToggleSpell))
                    action = A_CycleSpellRight;

                else
                {
                    MWWorld::Player& player = MWBase::Environment::get().getWorld()->getPlayer();
                    MWMechanics::DrawState_ state = player.getDrawState();
                    player.setAttackingOrSpell(currentValue != 0 && state != MWMechanics::DrawState_Nothing);
                }
            }
            else if (action == A_Jump)
            {
                if(joystickUsed && currentValue == 1.0 && actionIsActive(A_ToggleWeapon))
                    action = A_CycleWeaponLeft;

                else if (joystickUsed && currentValue == 1.0 && actionIsActive(A_ToggleSpell))
                    action = A_CycleSpellLeft;

                else
                    mActionManager->setAttemptJump(currentValue == 1.0 && previousValue == 0.0);
            }
        }

        if (currentValue == 1)
            mActionManager->executeAction(action);
    }

    void InputManager::updateCursorMode()
    {
        bool grab = !MWBase::Environment::get().getWindowManager()->containsMode(MWGui::GM_MainMenu)
             && !MWBase::Environment::get().getWindowManager()->isConsoleMode();

        bool was_relative = mInputWrapper->getMouseRelative();
        bool is_relative = !MWBase::Environment::get().getWindowManager()->isGuiMode();

        // don't keep the pointer away from the window edge in gui mode
        // stop using raw mouse motions and switch to system cursor movements
        mInputWrapper->setMouseRelative(is_relative);

        //we let the mouse escape in the main menu
        mInputWrapper->setGrabPointer(grab && (mGrabCursor || is_relative));

        //we switched to non-relative mode, move our cursor to where the in-game
        //cursor is
        if(!is_relative && was_relative != is_relative)
        {
            mMouseManager->warpMouse();
        }
    }

    void InputManager::update(float dt, bool disableControls, bool disableEvents)
    {
        mInputWrapper->setMouseVisible(MWBase::Environment::get().getWindowManager()->getCursorVisible());

        mInputWrapper->capture(disableEvents);

        if (disableControls)
        {
            updateCursorMode();
            return;
        }

        // update values of channels (as a result of pressed keys)
        mInputBinder->update(dt);

        updateCursorMode();

        bool controllerMove = mControllerManager->update(dt, disableControls);
        mKeyboardManager->update(dt, disableControls);
        mMouseManager->update(dt, disableControls);
        mSensorManager->update(dt, mGuiCursorEnabled);
        mActionManager->update(dt, controllerMove);
    }

    void InputManager::setDragDrop(bool dragDrop)
    {
        mDragDrop = dragDrop;
    }

    void InputManager::setGamepadGuiCursorEnabled(bool enabled)
    {
        mControllerManager->setGamepadGuiCursorEnabled(enabled);
    }

    void InputManager::changeInputMode(bool guiMode)
    {
        mGuiCursorEnabled = guiMode;
        mControllerManager->setGuiCursorEnabled(mGuiCursorEnabled);
        mMouseManager->setGuiCursorEnabled(mGuiCursorEnabled);
        mMouseManager->setMouseLookEnabled(!mGuiCursorEnabled);
        if (guiMode)
            MWBase::Environment::get().getWindowManager()->showCrosshair(false);
        MWBase::Environment::get().getWindowManager()->setCursorVisible(guiMode && (!mControllerManager->joystickLastUsed() || mControllerManager->gamepadGuiCursorEnabled()));
        // if not in gui mode, the camera decides whether to show crosshair or not.
    }

    void InputManager::processChangedSettings(const Settings::CategorySettingVector& changed)
    {
        for (Settings::CategorySettingVector::const_iterator it = changed.begin();
        it != changed.end(); ++it)
        {
            if (it->first == "Input" && it->second == "grab cursor")
                mGrabCursor = Settings::Manager::getBool("grab cursor", "Input");
        }

        mMouseManager->processChangedSettings(changed);
        mSensorManager->processChangedSettings(changed);
    }

    bool InputManager::getControlSwitch (const std::string& sw)
    {
        return mControlSwitch[sw];
    }

    void InputManager::toggleControlSwitch (const std::string& sw, bool value)
    {
        MWWorld::Player& player = MWBase::Environment::get().getWorld()->getPlayer();

        /// \note 7 switches at all, if-else is relevant
        if (sw == "playercontrols" && !value)
        {
            player.setLeftRight(0);
            player.setForwardBackward(0);
            player.setAutoMove(false);
            player.setUpDown(0);
        }
        else if (sw == "playerjumping" && !value)
        {
            /// \fixme maybe crouching at this time
            player.setUpDown(0);
        }
        else if (sw == "vanitymode")
        {
            MWBase::Environment::get().getWorld()->allowVanityMode(value);
        }
        else if (sw == "playerlooking" && !value)
        {
            MWBase::Environment::get().getWorld()->rotateObject(player.getPlayer(), 0.f, 0.f, 0.f);
        }
        mControlSwitch[sw] = value;
    }

    void InputManager::resetIdleTime()
    {
        mActionManager->resetIdleTime();
    }

    bool InputManager::actionIsActive (int id)
    {
        return (mInputBinder->getChannel (id)->getValue ()==1.0);
    }

    void InputManager::loadKeyDefaults (bool force)
    {
        // using hardcoded key defaults is inevitable, if we want the configuration files to stay valid
        // across different versions of OpenMW (in the case where another input action is added)
        std::map<int, SDL_Scancode> defaultKeyBindings;

        //Gets the Keyvalue from the Scancode; gives the button in the same place reguardless of keyboard format
        defaultKeyBindings[A_Activate] = SDL_SCANCODE_SPACE;
        defaultKeyBindings[A_MoveBackward] = SDL_SCANCODE_S;
        defaultKeyBindings[A_MoveForward] = SDL_SCANCODE_W;
        defaultKeyBindings[A_MoveLeft] = SDL_SCANCODE_A;
        defaultKeyBindings[A_MoveRight] = SDL_SCANCODE_D;
        defaultKeyBindings[A_ToggleWeapon] = SDL_SCANCODE_F;
        defaultKeyBindings[A_ToggleSpell] = SDL_SCANCODE_R;
        defaultKeyBindings[A_CycleSpellLeft] = SDL_SCANCODE_MINUS;
        defaultKeyBindings[A_CycleSpellRight] = SDL_SCANCODE_EQUALS;
        defaultKeyBindings[A_CycleWeaponLeft] = SDL_SCANCODE_LEFTBRACKET;
        defaultKeyBindings[A_CycleWeaponRight] = SDL_SCANCODE_RIGHTBRACKET;

        defaultKeyBindings[A_QuickKeysMenu] = SDL_SCANCODE_F1;
        defaultKeyBindings[A_Console] = SDL_SCANCODE_GRAVE;
        defaultKeyBindings[A_Run] = SDL_SCANCODE_LSHIFT;
        defaultKeyBindings[A_Sneak] = SDL_SCANCODE_LCTRL;
        defaultKeyBindings[A_AutoMove] = SDL_SCANCODE_Q;
        defaultKeyBindings[A_Jump] = SDL_SCANCODE_E;
        defaultKeyBindings[A_Journal] = SDL_SCANCODE_J;
        defaultKeyBindings[A_Rest] = SDL_SCANCODE_T;
        defaultKeyBindings[A_GameMenu] = SDL_SCANCODE_ESCAPE;
        defaultKeyBindings[A_TogglePOV] = SDL_SCANCODE_TAB;
        defaultKeyBindings[A_QuickKey1] = SDL_SCANCODE_1;
        defaultKeyBindings[A_QuickKey2] = SDL_SCANCODE_2;
        defaultKeyBindings[A_QuickKey3] = SDL_SCANCODE_3;
        defaultKeyBindings[A_QuickKey4] = SDL_SCANCODE_4;
        defaultKeyBindings[A_QuickKey5] = SDL_SCANCODE_5;
        defaultKeyBindings[A_QuickKey6] = SDL_SCANCODE_6;
        defaultKeyBindings[A_QuickKey7] = SDL_SCANCODE_7;
        defaultKeyBindings[A_QuickKey8] = SDL_SCANCODE_8;
        defaultKeyBindings[A_QuickKey9] = SDL_SCANCODE_9;
        defaultKeyBindings[A_QuickKey10] = SDL_SCANCODE_0;
        defaultKeyBindings[A_Screenshot] = SDL_SCANCODE_F12;
        defaultKeyBindings[A_ToggleHUD] = SDL_SCANCODE_F11;
        defaultKeyBindings[A_ToggleDebug] = SDL_SCANCODE_F10;
        defaultKeyBindings[A_AlwaysRun] = SDL_SCANCODE_CAPSLOCK;
        defaultKeyBindings[A_QuickSave] = SDL_SCANCODE_F5;
        defaultKeyBindings[A_QuickLoad] = SDL_SCANCODE_F9;

        std::map<int, int> defaultMouseButtonBindings;
        defaultMouseButtonBindings[A_Inventory] = SDL_BUTTON_RIGHT;
        defaultMouseButtonBindings[A_Use] = SDL_BUTTON_LEFT;

        std::map<int, ICS::InputControlSystem::MouseWheelClick> defaultMouseWheelBindings;
        defaultMouseWheelBindings[A_ZoomIn] = ICS::InputControlSystem::MouseWheelClick::UP;
        defaultMouseWheelBindings[A_ZoomOut] = ICS::InputControlSystem::MouseWheelClick::DOWN;

        for (int i = 0; i < A_Last; ++i)
        {
            ICS::Control* control;
            bool controlExists = mInputBinder->getChannel(i)->getControlsCount () != 0;
            if (!controlExists)
            {
                control = new ICS::Control(std::to_string(i), false, true, 0, ICS::ICS_MAX, ICS::ICS_MAX);
                mInputBinder->addControl(control);
                control->attachChannel(mInputBinder->getChannel(i), ICS::Channel::DIRECT);
            }
            else
            {
                control = mInputBinder->getChannel(i)->getAttachedControls ().front().control;
            }

            if (!controlExists || force ||
                    ( mInputBinder->getKeyBinding (control, ICS::Control::INCREASE) == SDL_SCANCODE_UNKNOWN
                      && mInputBinder->getMouseButtonBinding (control, ICS::Control::INCREASE) == ICS_MAX_DEVICE_BUTTONS
                      && mInputBinder->getMouseWheelBinding(control, ICS::Control::INCREASE) == ICS::InputControlSystem::MouseWheelClick::UNASSIGNED
                      ))
            {
                clearAllKeyBindings(control);

                if (defaultKeyBindings.find(i) != defaultKeyBindings.end()
                        && (force || !mInputBinder->isKeyBound(defaultKeyBindings[i])))
                {
                    control->setInitialValue(0.0f);
                    mInputBinder->addKeyBinding(control, defaultKeyBindings[i], ICS::Control::INCREASE);
                }
                else if (defaultMouseButtonBindings.find(i) != defaultMouseButtonBindings.end()
                         && (force || !mInputBinder->isMouseButtonBound(defaultMouseButtonBindings[i])))
                {
                    control->setInitialValue(0.0f);
                    mInputBinder->addMouseButtonBinding (control, defaultMouseButtonBindings[i], ICS::Control::INCREASE);
                }
                else if (defaultMouseWheelBindings.find(i) != defaultMouseWheelBindings.end()
                        && (force || !mInputBinder->isMouseWheelBound(defaultMouseWheelBindings[i])))
                {
                    control->setInitialValue(0.f);
                    mInputBinder->addMouseWheelBinding(control, defaultMouseWheelBindings[i], ICS::Control::INCREASE);
                }

                if (i == A_LookLeftRight && !mInputBinder->isKeyBound(SDL_SCANCODE_KP_4) && !mInputBinder->isKeyBound(SDL_SCANCODE_KP_6))
                {
                    mInputBinder->addKeyBinding(control, SDL_SCANCODE_KP_6, ICS::Control::INCREASE);
                    mInputBinder->addKeyBinding(control, SDL_SCANCODE_KP_4, ICS::Control::DECREASE);
                }
                if (i == A_LookUpDown && !mInputBinder->isKeyBound(SDL_SCANCODE_KP_8) && !mInputBinder->isKeyBound(SDL_SCANCODE_KP_2))
                {
                    mInputBinder->addKeyBinding(control, SDL_SCANCODE_KP_2, ICS::Control::INCREASE);
                    mInputBinder->addKeyBinding(control, SDL_SCANCODE_KP_8, ICS::Control::DECREASE);
                }
            }
        }
    }

    void InputManager::loadControllerDefaults(bool force)
    {
        // using hardcoded key defaults is inevitable, if we want the configuration files to stay valid
        // across different versions of OpenMW (in the case where another input action is added)
        std::map<int, int> defaultButtonBindings;

        defaultButtonBindings[A_Activate] = SDL_CONTROLLER_BUTTON_A;
        defaultButtonBindings[A_ToggleWeapon] = SDL_CONTROLLER_BUTTON_X;
        defaultButtonBindings[A_ToggleSpell] = SDL_CONTROLLER_BUTTON_Y;
        //defaultButtonBindings[A_QuickButtonsMenu] = SDL_GetButtonFromScancode(SDL_SCANCODE_F1); // Need to implement, should be ToggleSpell(5) AND Wait(9)
        defaultButtonBindings[A_Sneak] = SDL_CONTROLLER_BUTTON_LEFTSTICK;
        defaultButtonBindings[A_Journal] = SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
        defaultButtonBindings[A_Rest] = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
        defaultButtonBindings[A_TogglePOV] = SDL_CONTROLLER_BUTTON_RIGHTSTICK;
        defaultButtonBindings[A_Inventory] = SDL_CONTROLLER_BUTTON_B;
        defaultButtonBindings[A_GameMenu] = SDL_CONTROLLER_BUTTON_START;
        defaultButtonBindings[A_QuickSave] = SDL_CONTROLLER_BUTTON_GUIDE;
        defaultButtonBindings[A_MoveForward] = SDL_CONTROLLER_BUTTON_DPAD_UP;
        defaultButtonBindings[A_MoveLeft] = SDL_CONTROLLER_BUTTON_DPAD_LEFT;
        defaultButtonBindings[A_MoveBackward] = SDL_CONTROLLER_BUTTON_DPAD_DOWN;
        defaultButtonBindings[A_MoveRight] = SDL_CONTROLLER_BUTTON_DPAD_RIGHT;

        std::map<int, int> defaultAxisBindings;
        defaultAxisBindings[A_MoveForwardBackward] = SDL_CONTROLLER_AXIS_LEFTY;
        defaultAxisBindings[A_MoveLeftRight] = SDL_CONTROLLER_AXIS_LEFTX;
        defaultAxisBindings[A_LookUpDown] = SDL_CONTROLLER_AXIS_RIGHTY;
        defaultAxisBindings[A_LookLeftRight] = SDL_CONTROLLER_AXIS_RIGHTX;
        defaultAxisBindings[A_Use] = SDL_CONTROLLER_AXIS_TRIGGERRIGHT;
        defaultAxisBindings[A_Jump] = SDL_CONTROLLER_AXIS_TRIGGERLEFT;

        for (int i = 0; i < A_Last; i++)
        {
            ICS::Control* control;
            bool controlExists = mInputBinder->getChannel(i)->getControlsCount () != 0;
            if (!controlExists)
            {
                float initial;
                if (defaultAxisBindings.find(i) == defaultAxisBindings.end())
                    initial = 0.0f;
                else initial = 0.5f;
                control = new ICS::Control(std::to_string(i), false, true, initial, ICS::ICS_MAX, ICS::ICS_MAX);
                mInputBinder->addControl(control);
                control->attachChannel(mInputBinder->getChannel(i), ICS::Channel::DIRECT);
            }
            else
            {
                control = mInputBinder->getChannel(i)->getAttachedControls ().front().control;
            }

            if (!controlExists || force || ( mInputBinder->getJoystickAxisBinding (control, mFakeDeviceID, ICS::Control::INCREASE) == ICS::InputControlSystem::UNASSIGNED && mInputBinder->getJoystickButtonBinding (control, mFakeDeviceID, ICS::Control::INCREASE) == ICS_MAX_DEVICE_BUTTONS ))
            {
                clearAllControllerBindings(control);

                if (defaultButtonBindings.find(i) != defaultButtonBindings.end()
                        && (force || !mInputBinder->isJoystickButtonBound(mFakeDeviceID, defaultButtonBindings[i])))
                {
                    control->setInitialValue(0.0f);
                    mInputBinder->addJoystickButtonBinding(control, mFakeDeviceID, defaultButtonBindings[i], ICS::Control::INCREASE);
                }
                else if (defaultAxisBindings.find(i) != defaultAxisBindings.end() && (force || !mInputBinder->isJoystickAxisBound(mFakeDeviceID, defaultAxisBindings[i])))
                {
                    control->setValue(0.5f);
                    control->setInitialValue(0.5f);
                    mInputBinder->addJoystickAxisBinding(control, mFakeDeviceID, defaultAxisBindings[i], ICS::Control::INCREASE);
                }
            }
        }
    }

    std::string InputManager::getActionDescription (int action)
    {
        std::map<int, std::string> descriptions;

        if (action == A_Screenshot)
            return "Screenshot";
        else if (action == A_ZoomIn)
            return "Zoom In";
        else if (action == A_ZoomOut)
            return "Zoom Out";
        else if (action == A_ToggleHUD)
            return "Toggle HUD";

        descriptions[A_Use] = "sUse";
        descriptions[A_Activate] = "sActivate";
        descriptions[A_MoveBackward] = "sBack";
        descriptions[A_MoveForward] = "sForward";
        descriptions[A_MoveLeft] = "sLeft";
        descriptions[A_MoveRight] = "sRight";
        descriptions[A_ToggleWeapon] = "sReady_Weapon";
        descriptions[A_ToggleSpell] = "sReady_Magic";
        descriptions[A_CycleSpellLeft] = "sPrevSpell";
        descriptions[A_CycleSpellRight] = "sNextSpell";
        descriptions[A_CycleWeaponLeft] = "sPrevWeapon";
        descriptions[A_CycleWeaponRight] = "sNextWeapon";
        descriptions[A_Console] = "sConsoleTitle";
        descriptions[A_Run] = "sRun";
        descriptions[A_Sneak] = "sCrouch_Sneak";
        descriptions[A_AutoMove] = "sAuto_Run";
        descriptions[A_Jump] = "sJump";
        descriptions[A_Journal] = "sJournal";
        descriptions[A_Rest] = "sRestKey";
        descriptions[A_Inventory] = "sInventory";
        descriptions[A_TogglePOV] = "sTogglePOVCmd";
        descriptions[A_QuickKeysMenu] = "sQuickMenu";
        descriptions[A_QuickKey1] = "sQuick1Cmd";
        descriptions[A_QuickKey2] = "sQuick2Cmd";
        descriptions[A_QuickKey3] = "sQuick3Cmd";
        descriptions[A_QuickKey4] = "sQuick4Cmd";
        descriptions[A_QuickKey5] = "sQuick5Cmd";
        descriptions[A_QuickKey6] = "sQuick6Cmd";
        descriptions[A_QuickKey7] = "sQuick7Cmd";
        descriptions[A_QuickKey8] = "sQuick8Cmd";
        descriptions[A_QuickKey9] = "sQuick9Cmd";
        descriptions[A_QuickKey10] = "sQuick10Cmd";
        descriptions[A_AlwaysRun] = "sAlways_Run";
        descriptions[A_QuickSave] = "sQuickSaveCmd";
        descriptions[A_QuickLoad] = "sQuickLoadCmd";

        if (descriptions[action] == "")
            return ""; // not configurable

        return "#{" + descriptions[action] + "}";
    }

    std::string InputManager::getActionKeyBindingName (int action)
    {
        if (mInputBinder->getChannel (action)->getControlsCount () == 0)
            return "#{sNone}";

        ICS::Control* c = mInputBinder->getChannel (action)->getAttachedControls ().front().control;

        SDL_Scancode key = mInputBinder->getKeyBinding (c, ICS::Control::INCREASE);
        unsigned int mouse = mInputBinder->getMouseButtonBinding (c, ICS::Control::INCREASE);
        ICS::InputControlSystem::MouseWheelClick wheel = mInputBinder->getMouseWheelBinding(c, ICS::Control::INCREASE);
        if (key != SDL_SCANCODE_UNKNOWN)
            return MyGUI::TextIterator::toTagsString(mInputBinder->scancodeToString (key));
        else if (mouse != ICS_MAX_DEVICE_BUTTONS)
            return "#{sMouse} " + std::to_string(mouse);
        else if (wheel != ICS::InputControlSystem::MouseWheelClick::UNASSIGNED)
            switch (wheel)
            {
                case ICS::InputControlSystem::MouseWheelClick::UP:
                    return "Mouse Wheel Up";
                case ICS::InputControlSystem::MouseWheelClick::DOWN:
                    return "Mouse Wheel Down";
                case ICS::InputControlSystem::MouseWheelClick::RIGHT:
                    return "Mouse Wheel Right";
                case ICS::InputControlSystem::MouseWheelClick::LEFT:
                    return "Mouse Wheel Left";
                default:
                    return "#{sNone}";
            }
        else
            return "#{sNone}";
    }

    std::string InputManager::getActionControllerBindingName (int action)
    {
        if (mInputBinder->getChannel (action)->getControlsCount () == 0)
            return "#{sNone}";

        ICS::Control* c = mInputBinder->getChannel (action)->getAttachedControls ().front().control;

        if (mInputBinder->getJoystickAxisBinding (c, mFakeDeviceID, ICS::Control::INCREASE) != ICS::InputControlSystem::UNASSIGNED)
            return sdlControllerAxisToString(mInputBinder->getJoystickAxisBinding (c, mFakeDeviceID, ICS::Control::INCREASE));
        else if (mInputBinder->getJoystickButtonBinding (c, mFakeDeviceID, ICS::Control::INCREASE) != ICS_MAX_DEVICE_BUTTONS )
            return sdlControllerButtonToString(mInputBinder->getJoystickButtonBinding (c, mFakeDeviceID, ICS::Control::INCREASE));
        else
            return "#{sNone}";
    }

    std::vector<int> InputManager::getActionKeySorting()
    {
        std::vector<int> ret;
        ret.push_back(A_MoveForward);
        ret.push_back(A_MoveBackward);
        ret.push_back(A_MoveLeft);
        ret.push_back(A_MoveRight);
        ret.push_back(A_TogglePOV);
        ret.push_back(A_ZoomIn);
        ret.push_back(A_ZoomOut);
        ret.push_back(A_Run);
        ret.push_back(A_AlwaysRun);
        ret.push_back(A_Sneak);
        ret.push_back(A_Activate);
        ret.push_back(A_Use);
        ret.push_back(A_ToggleWeapon);
        ret.push_back(A_ToggleSpell);
        ret.push_back(A_CycleSpellLeft);
        ret.push_back(A_CycleSpellRight);
        ret.push_back(A_CycleWeaponLeft);
        ret.push_back(A_CycleWeaponRight);
        ret.push_back(A_AutoMove);
        ret.push_back(A_Jump);
        ret.push_back(A_Inventory);
        ret.push_back(A_Journal);
        ret.push_back(A_Rest);
        ret.push_back(A_Console);
        ret.push_back(A_QuickSave);
        ret.push_back(A_QuickLoad);
        ret.push_back(A_ToggleHUD);
        ret.push_back(A_Screenshot);
        ret.push_back(A_QuickKeysMenu);
        ret.push_back(A_QuickKey1);
        ret.push_back(A_QuickKey2);
        ret.push_back(A_QuickKey3);
        ret.push_back(A_QuickKey4);
        ret.push_back(A_QuickKey5);
        ret.push_back(A_QuickKey6);
        ret.push_back(A_QuickKey7);
        ret.push_back(A_QuickKey8);
        ret.push_back(A_QuickKey9);
        ret.push_back(A_QuickKey10);

        return ret;
    }
    std::vector<int> InputManager::getActionControllerSorting()
    {
        std::vector<int> ret;
        ret.push_back(A_TogglePOV);
        ret.push_back(A_ZoomIn);
        ret.push_back(A_ZoomOut);
        ret.push_back(A_Sneak);
        ret.push_back(A_Activate);
        ret.push_back(A_Use);
        ret.push_back(A_ToggleWeapon);
        ret.push_back(A_ToggleSpell);
        ret.push_back(A_AutoMove);
        ret.push_back(A_Jump);
        ret.push_back(A_Inventory);
        ret.push_back(A_Journal);
        ret.push_back(A_Rest);
        ret.push_back(A_QuickSave);
        ret.push_back(A_QuickLoad);
        ret.push_back(A_ToggleHUD);
        ret.push_back(A_Screenshot);
        ret.push_back(A_QuickKeysMenu);
        ret.push_back(A_QuickKey1);
        ret.push_back(A_QuickKey2);
        ret.push_back(A_QuickKey3);
        ret.push_back(A_QuickKey4);
        ret.push_back(A_QuickKey5);
        ret.push_back(A_QuickKey6);
        ret.push_back(A_QuickKey7);
        ret.push_back(A_QuickKey8);
        ret.push_back(A_QuickKey9);
        ret.push_back(A_QuickKey10);
        ret.push_back(A_CycleSpellLeft);
        ret.push_back(A_CycleSpellRight);
        ret.push_back(A_CycleWeaponLeft);
        ret.push_back(A_CycleWeaponRight);

        return ret;
    }

    void InputManager::enableDetectingBindingMode (int action, bool keyboard)
    {
        mDetectingKeyboard = keyboard;
        ICS::Control* c = mInputBinder->getChannel (action)->getAttachedControls ().front().control;
        mInputBinder->enableDetectingBindingState (c, ICS::Control::INCREASE);
    }

    void InputManager::keyBindingDetected(ICS::InputControlSystem* ICS, ICS::Control* control
        , SDL_Scancode key, ICS::Control::ControlChangingDirection direction)
    {
        //Disallow binding escape key
        if(key==SDL_SCANCODE_ESCAPE)
        {
            //Stop binding if esc pressed
            mInputBinder->cancelDetectingBindingState();
            MWBase::Environment::get().getWindowManager ()->notifyInputActionBound ();
            return;
        }

        // Disallow binding reserved keys
        if (key == SDL_SCANCODE_F3 || key == SDL_SCANCODE_F4 || key == SDL_SCANCODE_F10)
            return;

        #ifndef __APPLE__
        // Disallow binding Windows/Meta keys
        if (key == SDL_SCANCODE_LGUI || key == SDL_SCANCODE_RGUI)
            return;
        #endif

        if(!mDetectingKeyboard)
            return;

        clearAllKeyBindings(control);
        control->setInitialValue(0.0f);
        ICS::DetectingBindingListener::keyBindingDetected (ICS, control, key, direction);
        MWBase::Environment::get().getWindowManager ()->notifyInputActionBound ();
    }

    void InputManager::mouseAxisBindingDetected(ICS::InputControlSystem* ICS, ICS::Control* control
        , ICS::InputControlSystem::NamedAxis axis, ICS::Control::ControlChangingDirection direction)
    {
        // we don't want mouse movement bindings
        return;
    }

    void InputManager::mouseButtonBindingDetected(ICS::InputControlSystem* ICS, ICS::Control* control
        , unsigned int button, ICS::Control::ControlChangingDirection direction)
    {
        if(!mDetectingKeyboard)
            return;
        clearAllKeyBindings(control);
        control->setInitialValue(0.0f);
        ICS::DetectingBindingListener::mouseButtonBindingDetected (ICS, control, button, direction);
        MWBase::Environment::get().getWindowManager ()->notifyInputActionBound ();
    }

    void InputManager::mouseWheelBindingDetected(ICS::InputControlSystem* ICS, ICS::Control* control
        , ICS::InputControlSystem::MouseWheelClick click, ICS::Control::ControlChangingDirection direction)
    {
        if(!mDetectingKeyboard)
            return;
        clearAllKeyBindings(control);
        control->setInitialValue(0.0f);
        ICS::DetectingBindingListener::mouseWheelBindingDetected(ICS, control, click, direction);
        MWBase::Environment::get().getWindowManager()->notifyInputActionBound();
    }

    void InputManager::joystickAxisBindingDetected(ICS::InputControlSystem* ICS, int deviceID, ICS::Control* control
        , int axis, ICS::Control::ControlChangingDirection direction)
    {
        //only allow binding to the trigers
        if(axis != SDL_CONTROLLER_AXIS_TRIGGERLEFT && axis != SDL_CONTROLLER_AXIS_TRIGGERRIGHT)
            return;
        if(mDetectingKeyboard)
            return;

        clearAllControllerBindings(control);
        control->setValue(0.5f); //axis bindings must start at 0.5
        control->setInitialValue(0.5f);
        ICS::DetectingBindingListener::joystickAxisBindingDetected (ICS, deviceID, control, axis, direction);
        MWBase::Environment::get().getWindowManager ()->notifyInputActionBound ();
    }

    void InputManager::joystickButtonBindingDetected(ICS::InputControlSystem* ICS, int deviceID, ICS::Control* control
        , unsigned int button, ICS::Control::ControlChangingDirection direction)
    {
        if(mDetectingKeyboard)
            return;
        clearAllControllerBindings(control);
        control->setInitialValue(0.0f);
        ICS::DetectingBindingListener::joystickButtonBindingDetected (ICS, deviceID, control, button, direction);
        MWBase::Environment::get().getWindowManager ()->notifyInputActionBound ();
    }

    void InputManager::clearAllKeyBindings (ICS::Control* control)
    {
        // right now we don't really need multiple bindings for the same action, so remove all others first
        if (mInputBinder->getKeyBinding (control, ICS::Control::INCREASE) != SDL_SCANCODE_UNKNOWN)
            mInputBinder->removeKeyBinding (mInputBinder->getKeyBinding (control, ICS::Control::INCREASE));
        if (mInputBinder->getMouseButtonBinding (control, ICS::Control::INCREASE) != ICS_MAX_DEVICE_BUTTONS)
            mInputBinder->removeMouseButtonBinding (mInputBinder->getMouseButtonBinding (control, ICS::Control::INCREASE));
        if (mInputBinder->getMouseWheelBinding (control, ICS::Control::INCREASE) != ICS::InputControlSystem::MouseWheelClick::UNASSIGNED)
            mInputBinder->removeMouseWheelBinding (mInputBinder->getMouseWheelBinding(control, ICS::Control::INCREASE));
    }

    void InputManager::clearAllControllerBindings (ICS::Control* control)
    {
        // right now we don't really need multiple bindings for the same action, so remove all others first
        if (mInputBinder->getJoystickAxisBinding (control, mFakeDeviceID, ICS::Control::INCREASE) != SDL_SCANCODE_UNKNOWN)
            mInputBinder->removeJoystickAxisBinding (mFakeDeviceID, mInputBinder->getJoystickAxisBinding (control, mFakeDeviceID, ICS::Control::INCREASE));
        if (mInputBinder->getJoystickButtonBinding (control, mFakeDeviceID, ICS::Control::INCREASE) != ICS_MAX_DEVICE_BUTTONS)
            mInputBinder->removeJoystickButtonBinding (mFakeDeviceID, mInputBinder->getJoystickButtonBinding (control, mFakeDeviceID, ICS::Control::INCREASE));
    }

    int InputManager::countSavedGameRecords() const
    {
        return 1;
    }

    void InputManager::write(ESM::ESMWriter& writer, Loading::Listener& /*progress*/)
    {
        ESM::ControlsState controls;
        controls.mViewSwitchDisabled = !getControlSwitch("playerviewswitch");
        controls.mControlsDisabled = !getControlSwitch("playercontrols");
        controls.mJumpingDisabled = !getControlSwitch("playerjumping");
        controls.mLookingDisabled = !getControlSwitch("playerlooking");
        controls.mVanityModeDisabled = !getControlSwitch("vanitymode");
        controls.mWeaponDrawingDisabled = !getControlSwitch("playerfighting");
        controls.mSpellDrawingDisabled = !getControlSwitch("playermagic");

        writer.startRecord (ESM::REC_INPU);
        controls.save(writer);
        writer.endRecord (ESM::REC_INPU);
    }

    void InputManager::readRecord(ESM::ESMReader& reader, uint32_t type)
    {
        if (type == ESM::REC_INPU)
        {
            ESM::ControlsState controls;
            controls.load(reader);

            toggleControlSwitch("playerviewswitch", !controls.mViewSwitchDisabled);
            toggleControlSwitch("playercontrols", !controls.mControlsDisabled);
            toggleControlSwitch("playerjumping", !controls.mJumpingDisabled);
            toggleControlSwitch("playerlooking", !controls.mLookingDisabled);
            toggleControlSwitch("vanitymode", !controls.mVanityModeDisabled);
            toggleControlSwitch("playerfighting", !controls.mWeaponDrawingDisabled);
            toggleControlSwitch("playermagic", !controls.mSpellDrawingDisabled);
        }
    }

    void InputManager::resetToDefaultKeyBindings()
    {
        loadKeyDefaults(true);
    }

    void InputManager::resetToDefaultControllerBindings()
    {
        loadControllerDefaults(true);
    }

    void InputManager::setJoystickLastUsed(bool enabled)
    {
        mControllerManager->setJoystickLastUsed(enabled);
    }

    bool InputManager::joystickLastUsed()
    {
        return mControllerManager->joystickLastUsed();
    }
}
