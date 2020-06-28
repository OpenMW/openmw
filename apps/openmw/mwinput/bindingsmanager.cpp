#include "bindingsmanager.hpp"

#include <MyGUI_EditBox.h>

#include <extern/oics/ICSChannelListener.h>
#include <extern/oics/ICSInputControlSystem.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/inputmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/player.hpp"

#include "actions.hpp"
#include "sdlmappings.hpp"

namespace MWInput
{
    static const int sFakeDeviceId = 1; //As we only support one controller at a time, use a fake deviceID so we don't lose bindings when switching controllers

    void clearAllKeyBindings(ICS::InputControlSystem* inputBinder, ICS::Control* control)
    {
        // right now we don't really need multiple bindings for the same action, so remove all others first
        if (inputBinder->getKeyBinding(control, ICS::Control::INCREASE) != SDL_SCANCODE_UNKNOWN)
            inputBinder->removeKeyBinding(inputBinder->getKeyBinding(control, ICS::Control::INCREASE));
        if (inputBinder->getMouseButtonBinding(control, ICS::Control::INCREASE) != ICS_MAX_DEVICE_BUTTONS)
            inputBinder->removeMouseButtonBinding(inputBinder->getMouseButtonBinding(control, ICS::Control::INCREASE));
        if (inputBinder->getMouseWheelBinding(control, ICS::Control::INCREASE) != ICS::InputControlSystem::MouseWheelClick::UNASSIGNED)
            inputBinder->removeMouseWheelBinding(inputBinder->getMouseWheelBinding(control, ICS::Control::INCREASE));
    }

    void clearAllControllerBindings(ICS::InputControlSystem* inputBinder, ICS::Control* control)
    {
        // right now we don't really need multiple bindings for the same action, so remove all others first
        if (inputBinder->getJoystickAxisBinding(control, sFakeDeviceId, ICS::Control::INCREASE) != SDL_SCANCODE_UNKNOWN)
            inputBinder->removeJoystickAxisBinding(sFakeDeviceId, inputBinder->getJoystickAxisBinding(control, sFakeDeviceId, ICS::Control::INCREASE));
        if (inputBinder->getJoystickButtonBinding(control, sFakeDeviceId, ICS::Control::INCREASE) != ICS_MAX_DEVICE_BUTTONS)
            inputBinder->removeJoystickButtonBinding(sFakeDeviceId, inputBinder->getJoystickButtonBinding(control, sFakeDeviceId, ICS::Control::INCREASE));
    }

    class InputControlSystem : public ICS::InputControlSystem
    {
    public:
        InputControlSystem(const std::string& bindingsFile)
            : ICS::InputControlSystem(bindingsFile, true, nullptr, nullptr, A_Last)
        {
        }
    };

    class BindingsListener :
            public ICS::ChannelListener,
            public ICS::DetectingBindingListener
    {
    public:
        BindingsListener(ICS::InputControlSystem* inputBinder, BindingsManager* bindingsManager)
            : mInputBinder(inputBinder)
            , mBindingsManager(bindingsManager)
            , mDetectingKeyboard(false)
        {
        }

        virtual ~BindingsListener() = default;

        virtual void channelChanged(ICS::Channel* channel, float currentValue, float previousValue)
        {
            int action = channel->getNumber();
            mBindingsManager->actionValueChanged(action, currentValue, previousValue);
        }

        virtual void keyBindingDetected(ICS::InputControlSystem* ICS, ICS::Control* control
            , SDL_Scancode key, ICS::Control::ControlChangingDirection direction)
        {
            //Disallow binding escape key
            if (key==SDL_SCANCODE_ESCAPE)
            {
                //Stop binding if esc pressed
                mInputBinder->cancelDetectingBindingState();
                MWBase::Environment::get().getWindowManager()->notifyInputActionBound();
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

            if (!mDetectingKeyboard)
                return;

            clearAllKeyBindings(mInputBinder, control);
            control->setInitialValue(0.0f);
            ICS::DetectingBindingListener::keyBindingDetected(ICS, control, key, direction);
            MWBase::Environment::get().getWindowManager()->notifyInputActionBound();
        }

        virtual void mouseAxisBindingDetected(ICS::InputControlSystem* ICS, ICS::Control* control
            , ICS::InputControlSystem::NamedAxis axis, ICS::Control::ControlChangingDirection direction)
        {
            // we don't want mouse movement bindings
            return;
        }

        virtual void mouseButtonBindingDetected(ICS::InputControlSystem* ICS, ICS::Control* control
            , unsigned int button, ICS::Control::ControlChangingDirection direction)
        {
            if (!mDetectingKeyboard)
                return;
            clearAllKeyBindings(mInputBinder, control);
            control->setInitialValue(0.0f);
            ICS::DetectingBindingListener::mouseButtonBindingDetected(ICS, control, button, direction);
            MWBase::Environment::get().getWindowManager()->notifyInputActionBound();
        }

        virtual void mouseWheelBindingDetected(ICS::InputControlSystem* ICS, ICS::Control* control
            , ICS::InputControlSystem::MouseWheelClick click, ICS::Control::ControlChangingDirection direction)
        {
            if (!mDetectingKeyboard)
                return;
            clearAllKeyBindings(mInputBinder, control);
            control->setInitialValue(0.0f);
            ICS::DetectingBindingListener::mouseWheelBindingDetected(ICS, control, click, direction);
            MWBase::Environment::get().getWindowManager()->notifyInputActionBound();
        }

        virtual void joystickAxisBindingDetected(ICS::InputControlSystem* ICS, int deviceID, ICS::Control* control
            , int axis, ICS::Control::ControlChangingDirection direction)
        {
            //only allow binding to the trigers
            if (axis != SDL_CONTROLLER_AXIS_TRIGGERLEFT && axis != SDL_CONTROLLER_AXIS_TRIGGERRIGHT)
                return;
            if (mDetectingKeyboard)
                return;

            clearAllControllerBindings(mInputBinder, control);
            control->setValue(0.5f); //axis bindings must start at 0.5
            control->setInitialValue(0.5f);
            ICS::DetectingBindingListener::joystickAxisBindingDetected(ICS, deviceID, control, axis, direction);
            MWBase::Environment::get().getWindowManager()->notifyInputActionBound();
        }

        virtual void joystickButtonBindingDetected(ICS::InputControlSystem* ICS, int deviceID, ICS::Control* control
            , unsigned int button, ICS::Control::ControlChangingDirection direction)
        {
            if (mDetectingKeyboard)
                return;
            clearAllControllerBindings(mInputBinder,control);
            control->setInitialValue(0.0f);
            ICS::DetectingBindingListener::joystickButtonBindingDetected (ICS, deviceID, control, button, direction);
            MWBase::Environment::get().getWindowManager()->notifyInputActionBound();
        }

        void setDetectingKeyboard(bool detecting)
        {
            mDetectingKeyboard = detecting;
        }

    private:
        ICS::InputControlSystem* mInputBinder;
        BindingsManager* mBindingsManager;
        bool mDetectingKeyboard;
    };

    BindingsManager::BindingsManager(const std::string& userFile, bool userFileExists)
        : mUserFile(userFile)
        , mDragDrop(false)
    {
        std::string file = userFileExists ? userFile : "";
        mInputBinder = new InputControlSystem(file);
        mListener = new BindingsListener(mInputBinder, this);
        mInputBinder->setDetectingBindingListener(mListener);

        loadKeyDefaults();
        loadControllerDefaults();

        for (int i = 0; i < A_Last; ++i)
        {
            mInputBinder->getChannel(i)->addListener(mListener);
        }
    }

    void BindingsManager::setDragDrop(bool dragDrop)
    {
        mDragDrop = dragDrop;
    }

    BindingsManager::~BindingsManager()
    {
        mInputBinder->save(mUserFile);
        delete mInputBinder;
    }

    void BindingsManager::update(float dt)
    {
        // update values of channels (as a result of pressed keys)
        mInputBinder->update(dt);
    }

    bool BindingsManager::isLeftOrRightButton(int action, bool joystick) const
    {
        int mouseBinding = mInputBinder->getMouseButtonBinding(mInputBinder->getControl(action), ICS::Control::INCREASE);
        if (mouseBinding != ICS_MAX_DEVICE_BUTTONS)
            return true;
        int buttonBinding = mInputBinder->getJoystickButtonBinding(mInputBinder->getControl(action), sFakeDeviceId, ICS::Control::INCREASE);
        if (joystick && (buttonBinding == 0 || buttonBinding == 1))
            return true;
        return false;
    }

    void BindingsManager::setPlayerControlsEnabled(bool enabled)
    {
        int playerChannels[] = {A_AutoMove, A_AlwaysRun, A_ToggleWeapon,
                                A_ToggleSpell, A_Rest, A_QuickKey1, A_QuickKey2,
                                A_QuickKey3, A_QuickKey4, A_QuickKey5, A_QuickKey6,
                                A_QuickKey7, A_QuickKey8, A_QuickKey9, A_QuickKey10,
                                A_Use, A_Journal};

        for(int pc : playerChannels)
        {
            mInputBinder->getChannel(pc)->setEnabled(enabled);
        }
    }

    void BindingsManager::setJoystickDeadZone(float deadZone)
    {
        mInputBinder->setJoystickDeadZone(deadZone);
    }

    float BindingsManager::getActionValue (int id) const
    {
        return mInputBinder->getChannel(id)->getValue();
    }

    bool BindingsManager::actionIsActive (int id) const
    {
        return getActionValue(id) == 1.0;
    }

    void BindingsManager::loadKeyDefaults (bool force)
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
            bool controlExists = mInputBinder->getChannel(i)->getControlsCount() != 0;
            if (!controlExists)
            {
                control = new ICS::Control(std::to_string(i), false, true, 0, ICS::ICS_MAX, ICS::ICS_MAX);
                mInputBinder->addControl(control);
                control->attachChannel(mInputBinder->getChannel(i), ICS::Channel::DIRECT);
            }
            else
            {
                control = mInputBinder->getChannel(i)->getAttachedControls().front().control;
            }

            if (!controlExists || force ||
                    (mInputBinder->getKeyBinding(control, ICS::Control::INCREASE) == SDL_SCANCODE_UNKNOWN
                      && mInputBinder->getMouseButtonBinding(control, ICS::Control::INCREASE) == ICS_MAX_DEVICE_BUTTONS
                      && mInputBinder->getMouseWheelBinding(control, ICS::Control::INCREASE) == ICS::InputControlSystem::MouseWheelClick::UNASSIGNED))
            {
                clearAllKeyBindings(mInputBinder, control);

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
                    mInputBinder->addMouseButtonBinding(control, defaultMouseButtonBindings[i], ICS::Control::INCREASE);
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

    void BindingsManager::loadControllerDefaults(bool force)
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
            bool controlExists = mInputBinder->getChannel(i)->getControlsCount() != 0;
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
                control = mInputBinder->getChannel(i)->getAttachedControls().front().control;
            }

            if (!controlExists || force || (mInputBinder->getJoystickAxisBinding(control, sFakeDeviceId, ICS::Control::INCREASE) == ICS::InputControlSystem::UNASSIGNED &&
                mInputBinder->getJoystickButtonBinding(control, sFakeDeviceId, ICS::Control::INCREASE) == ICS_MAX_DEVICE_BUTTONS))
            {
                clearAllControllerBindings(mInputBinder, control);

                if (defaultButtonBindings.find(i) != defaultButtonBindings.end()
                        && (force || !mInputBinder->isJoystickButtonBound(sFakeDeviceId, defaultButtonBindings[i])))
                {
                    control->setInitialValue(0.0f);
                    mInputBinder->addJoystickButtonBinding(control, sFakeDeviceId, defaultButtonBindings[i], ICS::Control::INCREASE);
                }
                else if (defaultAxisBindings.find(i) != defaultAxisBindings.end() && (force || !mInputBinder->isJoystickAxisBound(sFakeDeviceId, defaultAxisBindings[i])))
                {
                    control->setValue(0.5f);
                    control->setInitialValue(0.5f);
                    mInputBinder->addJoystickAxisBinding(control, sFakeDeviceId, defaultAxisBindings[i], ICS::Control::INCREASE);
                }
            }
        }
    }

    std::string BindingsManager::getActionDescription(int action)
    {
        switch (action)
        {
            case A_Screenshot:
                return "Screenshot";
            case A_ZoomIn:
                return "Zoom In";
            case A_ZoomOut:
                return "Zoom Out";
            case A_ToggleHUD:
                return "Toggle HUD";
            case A_Use:
                return "#{sUse}";
            case A_Activate:
                return "#{sActivate}";
            case A_MoveBackward:
                return "#{sBack}";
            case A_MoveForward:
                return "#{sForward}";
            case A_MoveLeft:
                return "#{sLeft}";
            case A_MoveRight:
                return "#{sRight}";
            case A_ToggleWeapon:
                return "#{sReady_Weapon}";
            case A_ToggleSpell:
                return "#{sReady_Magic}";
            case A_CycleSpellLeft:
                return "#{sPrevSpell}";
            case A_CycleSpellRight:
                return "#{sNextSpell}";
            case A_CycleWeaponLeft:
                return "#{sPrevWeapon}";
            case A_CycleWeaponRight:
                return "#{sNextWeapon}";
            case A_Console:
                return "#{sConsoleTitle}";
            case A_Run:
                return "#{sRun}";
            case A_Sneak:
                return "#{sCrouch_Sneak}";
            case A_AutoMove:
                return "#{sAuto_Run}";
            case A_Jump:
                return "#{sJump}";
            case A_Journal:
                return "#{sJournal}";
            case A_Rest:
                return "#{sRestKey}";
            case A_Inventory:
                return "#{sInventory}";
            case A_TogglePOV:
                return "#{sTogglePOVCmd}";
            case A_QuickKeysMenu:
                return "#{sQuickMenu}";
            case A_QuickKey1:
                return "#{sQuick1Cmd}";
            case A_QuickKey2:
                return "#{sQuick2Cmd}";
            case A_QuickKey3:
                return "#{sQuick3Cmd}";
            case A_QuickKey4:
                return "#{sQuick4Cmd}";
            case A_QuickKey5:
                return "#{sQuick5Cmd}";
            case A_QuickKey6:
                return "#{sQuick6Cmd}";
            case A_QuickKey7:
                return "#{sQuick7Cmd}";
            case A_QuickKey8:
                return "#{sQuick8Cmd}";
            case A_QuickKey9:
                return "#{sQuick9Cmd}";
            case A_QuickKey10:
                return "#{sQuick10Cmd}";
            case A_AlwaysRun:
                return "#{sAlways_Run}";
            case A_QuickSave:
                return "#{sQuickSaveCmd}";
            case A_QuickLoad:
                return "#{sQuickLoadCmd}";
            default:
                return std::string(); // not configurable
        }
    }

    std::string BindingsManager::getActionKeyBindingName(int action)
    {
        if (mInputBinder->getChannel(action)->getControlsCount() == 0)
            return "#{sNone}";

        ICS::Control* c = mInputBinder->getChannel(action)->getAttachedControls().front().control;

        SDL_Scancode key = mInputBinder->getKeyBinding(c, ICS::Control::INCREASE);
        unsigned int mouse = mInputBinder->getMouseButtonBinding(c, ICS::Control::INCREASE);
        ICS::InputControlSystem::MouseWheelClick wheel = mInputBinder->getMouseWheelBinding(c, ICS::Control::INCREASE);
        if (key != SDL_SCANCODE_UNKNOWN)
            return MyGUI::TextIterator::toTagsString(mInputBinder->scancodeToString(key));
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

    std::string BindingsManager::getActionControllerBindingName(int action)
    {
        if (mInputBinder->getChannel(action)->getControlsCount() == 0)
            return "#{sNone}";

        ICS::Control* c = mInputBinder->getChannel(action)->getAttachedControls().front().control;

        if (mInputBinder->getJoystickAxisBinding(c, sFakeDeviceId, ICS::Control::INCREASE) != ICS::InputControlSystem::UNASSIGNED)
            return sdlControllerAxisToString(mInputBinder->getJoystickAxisBinding(c, sFakeDeviceId, ICS::Control::INCREASE));
        else if (mInputBinder->getJoystickButtonBinding(c, sFakeDeviceId, ICS::Control::INCREASE) != ICS_MAX_DEVICE_BUTTONS)
            return sdlControllerButtonToString(mInputBinder->getJoystickButtonBinding(c, sFakeDeviceId, ICS::Control::INCREASE));
        else
            return "#{sNone}";
    }

    std::vector<int> BindingsManager::getActionKeySorting()
    {
        static const std::vector<int> actions
        {
            A_MoveForward, A_MoveBackward, A_MoveLeft, A_MoveRight, A_TogglePOV, A_ZoomIn, A_ZoomOut,
            A_Run, A_AlwaysRun, A_Sneak, A_Activate, A_Use, A_ToggleWeapon, A_ToggleSpell,
            A_CycleSpellLeft, A_CycleSpellRight, A_CycleWeaponLeft, A_CycleWeaponRight, A_AutoMove,
            A_Jump, A_Inventory, A_Journal, A_Rest, A_Console, A_QuickSave, A_QuickLoad,
            A_ToggleHUD, A_Screenshot, A_QuickKeysMenu, A_QuickKey1, A_QuickKey2, A_QuickKey3,
            A_QuickKey4, A_QuickKey5, A_QuickKey6, A_QuickKey7, A_QuickKey8, A_QuickKey9, A_QuickKey10
        };

        return actions;
    }
    std::vector<int> BindingsManager::getActionControllerSorting()
    {
        static const std::vector<int> actions
        {
            A_TogglePOV, A_ZoomIn, A_ZoomOut, A_Sneak, A_Activate, A_Use, A_ToggleWeapon, A_ToggleSpell,
            A_AutoMove, A_Jump, A_Inventory, A_Journal, A_Rest, A_QuickSave, A_QuickLoad, A_ToggleHUD,
            A_Screenshot, A_QuickKeysMenu, A_QuickKey1, A_QuickKey2, A_QuickKey3, A_QuickKey4,
            A_QuickKey5, A_QuickKey6, A_QuickKey7, A_QuickKey8, A_QuickKey9, A_QuickKey10,
            A_CycleSpellLeft, A_CycleSpellRight, A_CycleWeaponLeft, A_CycleWeaponRight
        };

        return actions;
    }

    void BindingsManager::enableDetectingBindingMode(int action, bool keyboard)
    {
        mListener->setDetectingKeyboard(keyboard);
        ICS::Control* c = mInputBinder->getChannel(action)->getAttachedControls().front().control;
        mInputBinder->enableDetectingBindingState(c, ICS::Control::INCREASE);
    }

    bool BindingsManager::isDetectingBindingState() const
    {
        return mInputBinder->detectingBindingState();
    }

    void BindingsManager::mousePressed(const SDL_MouseButtonEvent &arg, int deviceID)
    {
        mInputBinder->mousePressed(arg, deviceID);
    }

    void BindingsManager::mouseReleased(const SDL_MouseButtonEvent &arg, int deviceID)
    {
        mInputBinder->mouseReleased(arg, deviceID);
    }

    void BindingsManager::mouseMoved(const SDLUtil::MouseMotionEvent &arg)
    {
        mInputBinder->mouseMoved(arg);
    }

    void BindingsManager::mouseWheelMoved(const SDL_MouseWheelEvent &arg)
    {
        mInputBinder->mouseWheelMoved(arg);
    }

    void BindingsManager::keyPressed(const SDL_KeyboardEvent &arg)
    {
        mInputBinder->keyPressed(arg);
    }

    void BindingsManager::keyReleased(const SDL_KeyboardEvent &arg)
    {
        mInputBinder->keyReleased(arg);
    }

    void BindingsManager::controllerAdded(int deviceID, const SDL_ControllerDeviceEvent &arg)
    {
        mInputBinder->controllerAdded(deviceID, arg);
    }

    void BindingsManager::controllerRemoved(const SDL_ControllerDeviceEvent &arg)
    {
        mInputBinder->controllerRemoved(arg);
    }

    void BindingsManager::controllerButtonPressed(int deviceID, const SDL_ControllerButtonEvent &arg)
    {
        mInputBinder->buttonPressed(deviceID, arg);
    }

    void BindingsManager::controllerButtonReleased(int deviceID, const SDL_ControllerButtonEvent &arg)
    {
        mInputBinder->buttonReleased(deviceID, arg);
    }

    void BindingsManager::controllerAxisMoved(int deviceID, const SDL_ControllerAxisEvent &arg)
    {
        mInputBinder->axisMoved(deviceID, arg);
    }

    SDL_Scancode BindingsManager::getKeyBinding(int actionId)
    {
        return mInputBinder->getKeyBinding(mInputBinder->getControl(actionId), ICS::Control::INCREASE);
    }

    void BindingsManager::actionValueChanged(int action, float currentValue, float previousValue)
    {
        MWBase::Environment::get().getInputManager()->resetIdleTime();

        if (mDragDrop && action != A_GameMenu && action != A_Inventory)
            return;

        if ((previousValue == 1 || previousValue == 0) && (currentValue==1 || currentValue==0))
        {
            //Is a normal button press, so don't change it at all
        }
        //Otherwise only trigger button presses as they go through specific points
        else if (previousValue >= 0.8 && currentValue < 0.8)
        {
            currentValue = 0.0;
            previousValue = 1.0;
        }
        else if (previousValue <= 0.6 && currentValue > 0.6)
        {
            currentValue = 1.0;
            previousValue = 0.0;
        }
        else
        {
            //If it's not switching between those values, ignore the channel change.
            return;
        }

        if (MWBase::Environment::get().getInputManager()->getControlSwitch("playercontrols"))
        {
            bool joystickUsed = MWBase::Environment::get().getInputManager()->joystickLastUsed();
            if (action == A_Use)
            {
                if (joystickUsed && currentValue == 1.0 && actionIsActive(A_ToggleWeapon))
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
                if (joystickUsed && currentValue == 1.0 && actionIsActive(A_ToggleWeapon))
                    action = A_CycleWeaponLeft;

                else if (joystickUsed && currentValue == 1.0 && actionIsActive(A_ToggleSpell))
                    action = A_CycleSpellLeft;

                else
                    MWBase::Environment::get().getInputManager()->setAttemptJump(currentValue == 1.0 && previousValue == 0.0);
            }
        }

        if (currentValue == 1)
            MWBase::Environment::get().getInputManager()->executeAction(action);
    }
}
