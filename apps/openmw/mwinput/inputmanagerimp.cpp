#include "inputmanagerimp.hpp"

#include <cmath>

#include <boost/lexical_cast.hpp>

#include <osgViewer/ViewerEventHandlers>

#include <MyGUI_InputManager.h>
#include <MyGUI_RenderManager.h>
#include <MyGUI_Widget.h>
#include <MyGUI_Button.h>
#include <MyGUI_EditBox.h>

#include <SDL_version.h>

#include <components/sdlutil/sdlinputwrapper.hpp>
#include <components/sdlutil/sdlvideowrapper.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/statemanager.hpp"
#include "../mwbase/environment.hpp"

#include "../mwworld/player.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/actorutil.hpp"

namespace MWInput
{
    InputManager::InputManager(
            SDL_Window* window,
            osg::ref_ptr<osgViewer::Viewer> viewer,
            osg::ref_ptr<osgViewer::ScreenCaptureHandler> screenCaptureHandler,
            const std::string& userFile, bool userFileExists,
            const std::string& controllerBindingsFile, bool grab)
        : mWindow(window)
        , mWindowVisible(true)
        , mViewer(viewer)
        , mScreenCaptureHandler(screenCaptureHandler)
        , mJoystickLastUsed(false)
        , mPlayer(NULL)
        , mInputManager(NULL)
        , mVideoWrapper(NULL)
        , mUserFile(userFile)
        , mDragDrop(false)
        , mGrabCursor (Settings::Manager::getBool("grab cursor", "Input"))
        , mInvertY (Settings::Manager::getBool("invert y axis", "Input"))
        , mControlsDisabled(false)
        , mCameraSensitivity (Settings::Manager::getFloat("camera sensitivity", "Input"))
        , mUISensitivity (Settings::Manager::getFloat("ui sensitivity", "Input"))
        , mCameraYMultiplier (Settings::Manager::getFloat("camera y multiplier", "Input"))
        , mPreviewPOVDelay(0.f)
        , mTimeIdle(0.f)
        , mMouseLookEnabled(false)
        , mGuiCursorEnabled(true)
        , mDetectingKeyboard(false)
        , mOverencumberedMessageDelay(0.f)
        , mGuiCursorX(0)
        , mGuiCursorY(0)
        , mMouseWheel(0)
        , mUserFileExists(userFileExists)
        , mAlwaysRunActive(Settings::Manager::getBool("always run", "Input"))
        , mSneakToggles(Settings::Manager::getBool("toggle sneak", "Input"))
        , mSneaking(false)
        , mAttemptJump(false)
        , mInvUiScalingFactor(1.f)
        , mFakeDeviceID(1)
    {
        mInputManager = new SDLUtil::InputWrapper(window, viewer, grab);
        mInputManager->setMouseEventCallback (this);
        mInputManager->setKeyboardEventCallback (this);
        mInputManager->setWindowEventCallback(this);
        mInputManager->setControllerEventCallback(this);

        mVideoWrapper = new SDLUtil::VideoWrapper(window, viewer);
        mVideoWrapper->setGammaContrast(Settings::Manager::getFloat("gamma", "Video"),
                                        Settings::Manager::getFloat("contrast", "Video"));

        std::string file = userFileExists ? userFile : "";
        mInputBinder = new ICS::InputControlSystem(file, true, this, NULL, A_Last);

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

        /* Joystick Init */

        // Load controller mappings
#if SDL_VERSION_ATLEAST(2,0,2)
        if(controllerBindingsFile!="")
        {
            SDL_GameControllerAddMappingsFromFile(controllerBindingsFile.c_str());
        }
#endif

        // Open all presently connected sticks
        int numSticks = SDL_NumJoysticks();
        for(int i = 0; i < numSticks; i++)
        {
            if(SDL_IsGameController(i))
            {
                SDL_ControllerDeviceEvent evt;
                evt.which = i;
                controllerAdded(mFakeDeviceID, evt);
            }
            else
            {
                //ICS_LOG(std::string("Unusable controller plugged in: ")+SDL_JoystickNameForIndex(i));
            }
        }

        float uiScale = Settings::Manager::getFloat("scaling factor", "GUI");
        if (uiScale != 0.f)
            mInvUiScalingFactor = 1.f / uiScale;

        int w,h;
        SDL_GetWindowSize(window, &w, &h);

        mGuiCursorX = mInvUiScalingFactor * w / 2.f;
        mGuiCursorY = mInvUiScalingFactor * h / 2.f;
    }

    void InputManager::clear()
    {
        // Enable all controls
        for (std::map<std::string, bool>::iterator it = mControlSwitch.begin(); it != mControlSwitch.end(); ++it)
            it->second = true;
    }

    InputManager::~InputManager()
    {
        mInputBinder->save (mUserFile);

        delete mInputBinder;

        delete mInputManager;

        delete mVideoWrapper;
    }

    bool InputManager::isWindowVisible()
    {
        return mWindowVisible;
    }

    void InputManager::setPlayerControlsEnabled(bool enabled)
    {
        int nPlayerChannels = 17;
        int playerChannels[] = {A_Activate, A_AutoMove, A_AlwaysRun, A_ToggleWeapon,
                                A_ToggleSpell, A_Rest, A_QuickKey1, A_QuickKey2,
                                A_QuickKey3, A_QuickKey4, A_QuickKey5, A_QuickKey6,
                                A_QuickKey7, A_QuickKey8, A_QuickKey9, A_QuickKey10,
                               A_Use};

        for(int i = 0; i < nPlayerChannels; i++) {
            int pc = playerChannels[i];
            mInputBinder->getChannel(pc)->setEnabled(enabled);
        }
    }

    void InputManager::channelChanged(ICS::Channel* channel, float currentValue, float previousValue)
    {
        if (mDragDrop)
            return;

        resetIdleTime ();

        int action = channel->getNumber();

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
            if (action == A_Use)
                mPlayer->setAttackingOrSpell(currentValue != 0);
            else if (action == A_Jump)
                mAttemptJump = (currentValue == 1.0 && previousValue == 0.0);
        }

        if (currentValue == 1)
        {
            // trigger action activated
            switch (action)
            {
            case A_GameMenu:
                if(!(MWBase::Environment::get().getStateManager()->getState() != MWBase::StateManager::State_Running
                    && MWBase::Environment::get().getWindowManager()->getMode() == MWGui::GM_MainMenu))
                        toggleMainMenu ();
                break;
            case A_Screenshot:
                screenshot();
                break;
            case A_Inventory:
                toggleInventory ();
                break;
            case A_Console:
                toggleConsole ();
                break;
            case A_Activate:
                resetIdleTime();
                if (!MWBase::Environment::get().getWindowManager()->isGuiMode())
                    activate();
                break;
            case A_Journal:
                toggleJournal ();
                break;
            case A_AutoMove:
                toggleAutoMove ();
                break;
            case A_AlwaysRun:
                toggleWalking ();
                break;
            case A_ToggleWeapon:
                toggleWeapon ();
                break;
            case A_Rest:
                rest();
                break;
            case A_ToggleSpell:
                toggleSpell ();
                break;
            case A_QuickKey1:
                quickKey(1);
                break;
            case A_QuickKey2:
                quickKey(2);
                break;
            case A_QuickKey3:
                quickKey(3);
                break;
            case A_QuickKey4:
                quickKey(4);
                break;
            case A_QuickKey5:
                quickKey(5);
                break;
            case A_QuickKey6:
                quickKey(6);
                break;
            case A_QuickKey7:
                quickKey(7);
                break;
            case A_QuickKey8:
                quickKey(8);
                break;
            case A_QuickKey9:
                quickKey(9);
                break;
            case A_QuickKey10:
                quickKey(10);
                break;
            case A_QuickKeysMenu:
                showQuickKeysMenu();
                break;
            case A_ToggleHUD:
                MWBase::Environment::get().getWindowManager()->toggleGui();
                break;
            case A_ToggleDebug:
                MWBase::Environment::get().getWindowManager()->toggleDebugWindow();
                break;
            case A_QuickSave:
                quickSave();
                break;
            case A_QuickLoad:
                quickLoad();
                break;
            case A_CycleSpellLeft:
                MWBase::Environment::get().getWindowManager()->cycleSpell(false);
                break;
            case A_CycleSpellRight:
                MWBase::Environment::get().getWindowManager()->cycleSpell(true);
                break;
            case A_CycleWeaponLeft:
                MWBase::Environment::get().getWindowManager()->cycleWeapon(false);
                break;
            case A_CycleWeaponRight:
                MWBase::Environment::get().getWindowManager()->cycleWeapon(true);
                break;
            case A_Sneak:
                if (mSneakToggles)
                {
                    toggleSneaking();
                }
                break;
            }
        }
    }

    void InputManager::updateCursorMode()
    {
        bool grab = !MWBase::Environment::get().getWindowManager()->containsMode(MWGui::GM_MainMenu)
             && MWBase::Environment::get().getWindowManager()->getMode() != MWGui::GM_Console;

        bool was_relative = mInputManager->getMouseRelative();
        bool is_relative = !MWBase::Environment::get().getWindowManager()->isGuiMode();

        // don't keep the pointer away from the window edge in gui mode
        // stop using raw mouse motions and switch to system cursor movements
        mInputManager->setMouseRelative(is_relative);

        //we let the mouse escape in the main menu
        mInputManager->setGrabPointer(grab && (mGrabCursor || is_relative));

        //we switched to non-relative mode, move our cursor to where the in-game
        //cursor is
        if( !is_relative && was_relative != is_relative )
        {
            mInputManager->warpMouse(static_cast<int>(mGuiCursorX/mInvUiScalingFactor), static_cast<int>(mGuiCursorY/mInvUiScalingFactor));
        }
    }

    void InputManager::update(float dt, bool disableControls, bool disableEvents)
    {
        mControlsDisabled = disableControls;

        mInputManager->setMouseVisible(MWBase::Environment::get().getWindowManager()->getCursorVisible());

        mInputManager->capture(disableEvents);
        // inject some fake mouse movement to force updating MyGUI's widget states
        MyGUI::InputManager::getInstance().injectMouseMove( int(mGuiCursorX), int(mGuiCursorY), mMouseWheel);

        if (mControlsDisabled)
        {
            updateCursorMode();
            return;
        }

        // update values of channels (as a result of pressed keys)
        mInputBinder->update(dt);

        updateCursorMode();

        if(mJoystickLastUsed)
        {
            if (mGuiCursorEnabled)
            {
                float xAxis = mInputBinder->getChannel(A_MoveLeftRight)->getValue()*2.0f-1.0f;
                float yAxis = mInputBinder->getChannel(A_MoveForwardBackward)->getValue()*2.0f-1.0f;
                float zAxis = mInputBinder->getChannel(A_LookUpDown)->getValue()*2.0f-1.0f;
                const MyGUI::IntSize& viewSize = MyGUI::RenderManager::getInstance().getViewSize();

                xAxis *= (1.5f - mInputBinder->getChannel(A_Use)->getValue());
                yAxis *= (1.5f - mInputBinder->getChannel(A_Use)->getValue());

                // We keep track of our own mouse position, so that moving the mouse while in
                // game mode does not move the position of the GUI cursor
                mGuiCursorX += xAxis * dt * 1500.0f * mInvUiScalingFactor;
                mGuiCursorY += yAxis * dt * 1500.0f * mInvUiScalingFactor;
                mMouseWheel -= static_cast<int>(zAxis * dt * 1500.0f);

                mGuiCursorX = std::max(0.f, std::min(mGuiCursorX, float(viewSize.width)));
                mGuiCursorY = std::max(0.f, std::min(mGuiCursorY, float(viewSize.height)));

                MyGUI::InputManager::getInstance().injectMouseMove(static_cast<int>(mGuiCursorX), static_cast<int>(mGuiCursorY), mMouseWheel);
                mInputManager->warpMouse(static_cast<int>(mGuiCursorX/mInvUiScalingFactor), static_cast<int>(mGuiCursorY/mInvUiScalingFactor));
            }
            if (mMouseLookEnabled)
            {
                float xAxis = mInputBinder->getChannel(A_LookLeftRight)->getValue()*2.0f-1.0f;
                float yAxis = mInputBinder->getChannel(A_LookUpDown)->getValue()*2.0f-1.0f;
                resetIdleTime();

                float rot[3];
                rot[0] = yAxis * (dt * 100.0f) * 10.0f * mCameraSensitivity * (1.0f/256.f) * (mInvertY ? -1 : 1) * mCameraYMultiplier;
                rot[1] = 0.0f;
                rot[2] = xAxis * (dt * 100.0f) * 10.0f * mCameraSensitivity * (1.0f/256.f);

                // Only actually turn player when we're not in vanity mode
                if(!MWBase::Environment::get().getWorld()->vanityRotateCamera(rot))
                {
                    mPlayer->yaw(rot[2]);
                    mPlayer->pitch(rot[0]);
                }
            }
        }

        // Disable movement in Gui mode
        if (!(MWBase::Environment::get().getWindowManager()->isGuiMode()
            || MWBase::Environment::get().getStateManager()->getState() != MWBase::StateManager::State_Running))
        {
            // Configure player movement according to keyboard input. Actual movement will
            // be done in the physics system.
            if (mControlSwitch["playercontrols"])
            {
                bool triedToMove = false;
                bool isRunning = false;
                if(mJoystickLastUsed)
                {
                    float xAxis = mInputBinder->getChannel(A_MoveLeftRight)->getValue();
                    float yAxis = mInputBinder->getChannel(A_MoveForwardBackward)->getValue();
                    if (xAxis < .5)
                    {
                        triedToMove = true;
                        mPlayer->setLeftRight (-1);
                    }
                    else if (xAxis > .5)
                    {
                        triedToMove = true;
                        mPlayer->setLeftRight (1);
                    }

                    if (yAxis < .5)
                    {
                        triedToMove = true;
                        mPlayer->setAutoMove (false);
                        mPlayer->setForwardBackward (1);
                    }
                    else if (yAxis > .5)
                    {
                        triedToMove = true;
                        mPlayer->setAutoMove (false);
                        mPlayer->setForwardBackward (-1);
                    }

                    else if(mPlayer->getAutoMove())
                    {
                        triedToMove = true;
                        mPlayer->setForwardBackward (1);
                    }
                    isRunning = xAxis > .75 || xAxis < .25 || yAxis > .75 || yAxis < .25;
                    if(triedToMove) resetIdleTime();
                }
                else
                {
                    if (actionIsActive(A_MoveLeft))
                    {
                        triedToMove = true;
                        mPlayer->setLeftRight (-1);
                    }
                    else if (actionIsActive(A_MoveRight))
                    {
                        triedToMove = true;
                        mPlayer->setLeftRight (1);
                    }

                    if (actionIsActive(A_MoveForward))
                    {
                        triedToMove = true;
                        mPlayer->setAutoMove (false);
                        mPlayer->setForwardBackward (1);
                    }
                    else if (actionIsActive(A_MoveBackward))
                    {
                        triedToMove = true;
                        mPlayer->setAutoMove (false);
                        mPlayer->setForwardBackward (-1);
                    }

                    else if(mPlayer->getAutoMove())
                    {
                        triedToMove = true;
                        mPlayer->setForwardBackward (1);
                    }
                }

                if (!mSneakToggles)
                {
                    mPlayer->setSneak(actionIsActive(A_Sneak));
                }

                if (mAttemptJump && mControlSwitch["playerjumping"])
                {
                    mPlayer->setUpDown (1);
                    triedToMove = true;
                    mOverencumberedMessageDelay = 0.f;
                }

                if (mAlwaysRunActive || isRunning)
                    mPlayer->setRunState(!actionIsActive(A_Run));
                else
                    mPlayer->setRunState(actionIsActive(A_Run));

                // if player tried to start moving, but can't (due to being overencumbered), display a notification.
                if (triedToMove)
                {
                    MWWorld::Ptr player = MWBase::Environment::get().getWorld ()->getPlayerPtr();
                    mOverencumberedMessageDelay -= dt;
                    if (player.getClass().getEncumbrance(player) > player.getClass().getCapacity(player))
                    {
                        mPlayer->setAutoMove (false);
                        if (mOverencumberedMessageDelay <= 0)
                        {
                            MWBase::Environment::get().getWindowManager ()->messageBox("#{sNotifyMessage59}");
                            mOverencumberedMessageDelay = 1.0;
                        }
                    }
                }

                if (mControlSwitch["playerviewswitch"]) {

                    if (actionIsActive(A_TogglePOV)) {
                        if (mPreviewPOVDelay <= 0.5 &&
                            (mPreviewPOVDelay += dt) > 0.5)
                        {
                            mPreviewPOVDelay = 1.f;
                            MWBase::Environment::get().getWorld()->togglePreviewMode(true);
                        }
                    } else {
                        //disable preview mode
                        MWBase::Environment::get().getWorld()->togglePreviewMode(false);
                        if (mPreviewPOVDelay > 0.f && mPreviewPOVDelay <= 0.5) {
                            MWBase::Environment::get().getWorld()->togglePOV();
                        }
                        mPreviewPOVDelay = 0.f;
                    }
                }
            }
            if (actionIsActive(A_MoveForward) ||
                actionIsActive(A_MoveBackward) ||
                actionIsActive(A_MoveLeft) ||
                actionIsActive(A_MoveRight) ||
                actionIsActive(A_Jump) ||
                actionIsActive(A_Sneak) ||
                actionIsActive(A_TogglePOV))
            {
                resetIdleTime();
            } else {
                updateIdleTime(dt);
            }
        }
        mAttemptJump = false; // Can only jump on first frame input is on
    }

    void InputManager::setDragDrop(bool dragDrop)
    {
        mDragDrop = dragDrop;
    }

    void InputManager::changeInputMode(bool guiMode)
    {
        mGuiCursorEnabled = guiMode;
        mMouseLookEnabled = !guiMode;
        if (guiMode)
            MWBase::Environment::get().getWindowManager()->showCrosshair(false);
        MWBase::Environment::get().getWindowManager()->setCursorVisible(guiMode);
        // if not in gui mode, the camera decides whether to show crosshair or not.
    }

    void InputManager::processChangedSettings(const Settings::CategorySettingVector& changed)
    {
        bool changeRes = false;

        for (Settings::CategorySettingVector::const_iterator it = changed.begin();
        it != changed.end(); ++it)
        {
            if (it->first == "Input" && it->second == "invert y axis")
                mInvertY = Settings::Manager::getBool("invert y axis", "Input");

            if (it->first == "Input" && it->second == "camera sensitivity")
                mCameraSensitivity = Settings::Manager::getFloat("camera sensitivity", "Input");

            if (it->first == "Input" && it->second == "ui sensitivity")
                mUISensitivity = Settings::Manager::getFloat("ui sensitivity", "Input");

            if (it->first == "Input" && it->second == "grab cursor")
                mGrabCursor = Settings::Manager::getBool("grab cursor", "Input");

            if (it->first == "Video" && (
                    it->second == "resolution x"
                    || it->second == "resolution y"
                    || it->second == "fullscreen"
                    || it->second == "window border"))
                changeRes = true;

            if (it->first == "Video" && it->second == "vsync")
                mVideoWrapper->setSyncToVBlank(Settings::Manager::getBool("vsync", "Video"));

            if (it->first == "Video" && (it->second == "gamma" || it->second == "contrast"))
                mVideoWrapper->setGammaContrast(Settings::Manager::getFloat("gamma", "Video"),
                                                Settings::Manager::getFloat("contrast", "Video"));
        }

        if (changeRes)
        {
            mVideoWrapper->setVideoMode(Settings::Manager::getInt("resolution x", "Video"),
                                        Settings::Manager::getInt("resolution y", "Video"),
                                        Settings::Manager::getBool("fullscreen", "Video"),
                                        Settings::Manager::getBool("window border", "Video"));
        }
    }

    bool InputManager::getControlSwitch (const std::string& sw)
    {
        return mControlSwitch[sw];
    }

    void InputManager::toggleControlSwitch (const std::string& sw, bool value)
    {
        if (mControlSwitch[sw] == value) {
            return;
        }
        /// \note 7 switches at all, if-else is relevant
        if (sw == "playercontrols" && !value) {
            mPlayer->setLeftRight(0);
            mPlayer->setForwardBackward(0);
            mPlayer->setAutoMove(false);
            mPlayer->setUpDown(0);
        } else if (sw == "playerjumping" && !value) {
            /// \fixme maybe crouching at this time
            mPlayer->setUpDown(0);
        } else if (sw == "vanitymode") {
            MWBase::Environment::get().getWorld()->allowVanityMode(value);
        } else if (sw == "playerlooking") {
            MWBase::Environment::get().getWorld()->togglePlayerLooking(value);
        }
        mControlSwitch[sw] = value;
    }

    void InputManager::keyPressed( const SDL_KeyboardEvent &arg )
    {
        // HACK: to make Morrowind's default keybinding for the console work without printing an extra "^" upon closing
        // This assumes that SDL_TextInput events always come *after* the key event
        // (which is somewhat reasonable, and hopefully true for all SDL platforms)
        OIS::KeyCode kc = mInputManager->sdl2OISKeyCode(arg.keysym.sym);
        if (mInputBinder->getKeyBinding(mInputBinder->getControl(A_Console), ICS::Control::INCREASE)
                == arg.keysym.scancode
                && MWBase::Environment::get().getWindowManager()->getMode() == MWGui::GM_Console)
            SDL_StopTextInput();

        bool consumed = false;
        if (kc != OIS::KC_UNASSIGNED)
        {
            consumed = SDL_IsTextInputActive() &&
                    ( !(SDLK_SCANCODE_MASK & arg.keysym.sym) && std::isprint(arg.keysym.sym)); // Little trick to check if key is printable
            bool guiFocus = MyGUI::InputManager::getInstance().injectKeyPress(MyGUI::KeyCode::Enum(kc), 0);
            setPlayerControlsEnabled(!guiFocus);
        }
        if (!mControlsDisabled && !consumed)
            mInputBinder->keyPressed (arg);
        mJoystickLastUsed = false;
    }

    void InputManager::textInput(const SDL_TextInputEvent &arg)
    {
        MyGUI::UString ustring(&arg.text[0]);
        MyGUI::UString::utf32string utf32string = ustring.asUTF32();
        for (MyGUI::UString::utf32string::const_iterator it = utf32string.begin(); it != utf32string.end(); ++it)
            MyGUI::InputManager::getInstance().injectKeyPress(MyGUI::KeyCode::None, *it);
    }

    void InputManager::keyReleased(const SDL_KeyboardEvent &arg )
    {
        mJoystickLastUsed = false;
        OIS::KeyCode kc = mInputManager->sdl2OISKeyCode(arg.keysym.sym);

        setPlayerControlsEnabled(!MyGUI::InputManager::getInstance().injectKeyRelease(MyGUI::KeyCode::Enum(kc)));
        mInputBinder->keyReleased (arg);
    }

    void InputManager::mousePressed( const SDL_MouseButtonEvent &arg, Uint8 id )
    {
        mJoystickLastUsed = false;
        bool guiMode = false;

        if (id == SDL_BUTTON_LEFT || id == SDL_BUTTON_RIGHT) // MyGUI only uses these mouse events
        {
            guiMode = MWBase::Environment::get().getWindowManager()->isGuiMode();
            guiMode = MyGUI::InputManager::getInstance().injectMousePress(static_cast<int>(mGuiCursorX), static_cast<int>(mGuiCursorY), sdlButtonToMyGUI(id)) && guiMode;
            if (MyGUI::InputManager::getInstance ().getMouseFocusWidget () != 0)
            {
                MyGUI::Button* b = MyGUI::InputManager::getInstance ().getMouseFocusWidget ()->castType<MyGUI::Button>(false);
                if (b && b->getEnabled())
                {
                    MWBase::Environment::get().getSoundManager ()->playSound ("Menu Click", 1.f, 1.f);
                }
            }
        }

        setPlayerControlsEnabled(!guiMode);

        // Don't trigger any mouse bindings while in settings menu, otherwise rebinding controls becomes impossible
        if (MWBase::Environment::get().getWindowManager()->getMode() != MWGui::GM_Settings)
            mInputBinder->mousePressed (arg, id);
    }

    void InputManager::mouseReleased( const SDL_MouseButtonEvent &arg, Uint8 id )
    {
        mJoystickLastUsed = false;

        if(mInputBinder->detectingBindingState())
        {
            mInputBinder->mouseReleased (arg, id);
        } else {
            bool guiMode = MWBase::Environment::get().getWindowManager()->isGuiMode();
            guiMode = MyGUI::InputManager::getInstance().injectMouseRelease(static_cast<int>(mGuiCursorX), static_cast<int>(mGuiCursorY), sdlButtonToMyGUI(id)) && guiMode;

            if(mInputBinder->detectingBindingState()) return; // don't allow same mouseup to bind as initiated bind

            setPlayerControlsEnabled(!guiMode);
            mInputBinder->mouseReleased (arg, id);
        }
    }

    void InputManager::mouseMoved(const SDLUtil::MouseMotionEvent &arg )
    {
        mInputBinder->mouseMoved (arg);

        mJoystickLastUsed = false;
        resetIdleTime ();

        if (mGuiCursorEnabled)
        {
            // We keep track of our own mouse position, so that moving the mouse while in
            // game mode does not move the position of the GUI cursor
            mGuiCursorX = static_cast<float>(arg.x) * mInvUiScalingFactor;
            mGuiCursorY = static_cast<float>(arg.y) * mInvUiScalingFactor;

            mMouseWheel = int(arg.z);

            MyGUI::InputManager::getInstance().injectMouseMove( int(mGuiCursorX), int(mGuiCursorY), mMouseWheel);
        }

        if (mMouseLookEnabled && !mControlsDisabled)
        {
            resetIdleTime();

            float x = arg.xrel * mCameraSensitivity * (1.0f/256.f);
            float y = arg.yrel * mCameraSensitivity * (1.0f/256.f) * (mInvertY ? -1 : 1) * mCameraYMultiplier;

            float rot[3];
            rot[0] = -y;
            rot[1] = 0.0f;
            rot[2] = -x;

            // Only actually turn player when we're not in vanity mode
            if(!MWBase::Environment::get().getWorld()->vanityRotateCamera(rot))
            {
                mPlayer->yaw(x);
                mPlayer->pitch(y);
            }

            if (arg.zrel && mControlSwitch["playerviewswitch"] && mControlSwitch["playercontrols"]) //Check to make sure you are allowed to zoomout and there is a change
            {
                MWBase::Environment::get().getWorld()->changeVanityModeScale(static_cast<float>(arg.zrel));

                if (Settings::Manager::getBool("allow third person zoom", "Input"))
                    MWBase::Environment::get().getWorld()->setCameraDistance(static_cast<float>(arg.zrel), true, true);
            }
        }
    }

    void InputManager::buttonPressed(int deviceID, const SDL_ControllerButtonEvent &arg )
    {
        mJoystickLastUsed = true;
        bool guiMode = false;

        if (arg.button == SDL_CONTROLLER_BUTTON_A || arg.button == SDL_CONTROLLER_BUTTON_B) // We'll pretend that A is left click and B is right click
        {
            guiMode = MWBase::Environment::get().getWindowManager()->isGuiMode();
            if(!mInputBinder->detectingBindingState())
            {
                guiMode = MyGUI::InputManager::getInstance().injectMousePress(static_cast<int>(mGuiCursorX), static_cast<int>(mGuiCursorY),
                    sdlButtonToMyGUI((arg.button == SDL_CONTROLLER_BUTTON_B) ? SDL_BUTTON_RIGHT : SDL_BUTTON_LEFT)) && guiMode;
                if (MyGUI::InputManager::getInstance ().getMouseFocusWidget () != 0)
                {
                    MyGUI::Button* b = MyGUI::InputManager::getInstance ().getMouseFocusWidget ()->castType<MyGUI::Button>(false);
                    if (b && b->getEnabled())
                    {
                        MWBase::Environment::get().getSoundManager ()->playSound ("Menu Click", 1.f, 1.f);
                    }
                }
            }
        }

        setPlayerControlsEnabled(!guiMode);

        //esc, to leave initial movie screen
        OIS::KeyCode kc = mInputManager->sdl2OISKeyCode(SDLK_ESCAPE);
        bool guiFocus = MyGUI::InputManager::getInstance().injectKeyPress(MyGUI::KeyCode::Enum(kc), 0);
        setPlayerControlsEnabled(!guiFocus);

        if (!mControlsDisabled)
            mInputBinder->buttonPressed(deviceID, arg);
    }

    void InputManager::buttonReleased(int deviceID, const SDL_ControllerButtonEvent &arg )
    {
        mJoystickLastUsed = true;
        if(mInputBinder->detectingBindingState())
            mInputBinder->buttonReleased(deviceID, arg);
        else if(arg.button == SDL_CONTROLLER_BUTTON_A || arg.button == SDL_CONTROLLER_BUTTON_B)
        {
            bool guiMode = MWBase::Environment::get().getWindowManager()->isGuiMode();
            guiMode = MyGUI::InputManager::getInstance().injectMouseRelease(static_cast<int>(mGuiCursorX), static_cast<int>(mGuiCursorY), sdlButtonToMyGUI((arg.button == SDL_CONTROLLER_BUTTON_B) ? SDL_BUTTON_RIGHT : SDL_BUTTON_LEFT)) && guiMode;

            if(mInputBinder->detectingBindingState()) return; // don't allow same mouseup to bind as initiated bind

            setPlayerControlsEnabled(!guiMode);
            mInputBinder->buttonReleased(deviceID, arg);
        }
        else
            mInputBinder->buttonReleased(deviceID, arg);

        ///to escape initial movie
        OIS::KeyCode kc = mInputManager->sdl2OISKeyCode(SDLK_ESCAPE);
        setPlayerControlsEnabled(!MyGUI::InputManager::getInstance().injectKeyRelease(MyGUI::KeyCode::Enum(kc)));
    }

    void InputManager::axisMoved(int deviceID, const SDL_ControllerAxisEvent &arg )
    {
        mJoystickLastUsed = true;
        if (!mControlsDisabled)
            mInputBinder->axisMoved(deviceID, arg);
    }

    void InputManager::controllerAdded(int deviceID, const SDL_ControllerDeviceEvent &arg)
    {
        mInputBinder->controllerAdded(deviceID, arg);
    }
    void InputManager::controllerRemoved(const SDL_ControllerDeviceEvent &arg)
    {
        mInputBinder->controllerRemoved(arg);
    }

    void InputManager::windowFocusChange(bool have_focus)
    {
    }

    void InputManager::windowVisibilityChange(bool visible)
    {
        mWindowVisible = visible;
    }

    void InputManager::windowResized(int x, int y)
    {
        Settings::Manager::setInt("resolution x", "Video", x);
        Settings::Manager::setInt("resolution y", "Video", y);

        MWBase::Environment::get().getWindowManager()->windowResized(x, y);
    }

    void InputManager::windowClosed()
    {
        MWBase::Environment::get().getStateManager()->requestQuit();
    }

    void InputManager::toggleMainMenu()
    {
        if (MyGUI::InputManager::getInstance().isModalAny()) {
            MWBase::Environment::get().getWindowManager()->exitCurrentModal();
            return;
        }

        if(!MWBase::Environment::get().getWindowManager()->isGuiMode()) //No open GUIs, open up the MainMenu
        {
            MWBase::Environment::get().getWindowManager()->pushGuiMode (MWGui::GM_MainMenu);
        }
        else //Close current GUI
        {
            MWBase::Environment::get().getWindowManager()->exitCurrentGuiMode();
        }
    }

    void InputManager::quickLoad() {
        if (!MyGUI::InputManager::getInstance().isModalAny())
            MWBase::Environment::get().getStateManager()->quickLoad();
    }

    void InputManager::quickSave() {
        if (!MyGUI::InputManager::getInstance().isModalAny())
            MWBase::Environment::get().getStateManager()->quickSave();
    }
    void InputManager::toggleSpell()
    {
        if (MWBase::Environment::get().getWindowManager()->isGuiMode()) return;

        // Not allowed before the magic window is accessible
        if (!mControlSwitch["playermagic"] || !mControlSwitch["playercontrols"])
            return;

        // Not allowed if no spell selected
        MWWorld::InventoryStore& inventory = mPlayer->getPlayer().getClass().getInventoryStore(mPlayer->getPlayer());
        if (MWBase::Environment::get().getWindowManager()->getSelectedSpell().empty() &&
            inventory.getSelectedEnchantItem() == inventory.end())
            return;

        MWMechanics::DrawState_ state = mPlayer->getDrawState();
        if (state == MWMechanics::DrawState_Weapon || state == MWMechanics::DrawState_Nothing)
            mPlayer->setDrawState(MWMechanics::DrawState_Spell);
        else
            mPlayer->setDrawState(MWMechanics::DrawState_Nothing);
    }

    void InputManager::toggleWeapon()
    {
        if (MWBase::Environment::get().getWindowManager()->isGuiMode()) return;

        // Not allowed before the inventory window is accessible
        if (!mControlSwitch["playerfighting"] || !mControlSwitch["playercontrols"])
            return;

        MWMechanics::DrawState_ state = mPlayer->getDrawState();
        if (state == MWMechanics::DrawState_Spell || state == MWMechanics::DrawState_Nothing)
            mPlayer->setDrawState(MWMechanics::DrawState_Weapon);
        else
            mPlayer->setDrawState(MWMechanics::DrawState_Nothing);
    }

    void InputManager::rest()
    {
        if (!mControlSwitch["playercontrols"])
            return;

        if (!MWBase::Environment::get().getWindowManager()->getRestEnabled () || MWBase::Environment::get().getWindowManager()->isGuiMode ())
            return;

        if(mPlayer->isInCombat()) {//Check if in combat
            MWBase::Environment::get().getWindowManager()->messageBox("#{sNotifyMessage2}"); //Nope,
            return;
        }
        MWBase::Environment::get().getWindowManager()->pushGuiMode (MWGui::GM_Rest); //Open rest GUI

    }

    void InputManager::screenshot()
    {
        mScreenCaptureHandler->setFramesToCapture(1);
        mScreenCaptureHandler->captureNextFrame(*mViewer);

        MWBase::Environment::get().getWindowManager()->messageBox ("Screenshot saved");
    }

    void InputManager::toggleInventory()
    {
        if (!mControlSwitch["playercontrols"])
            return;

        if (MyGUI::InputManager::getInstance ().isModalAny())
            return;

        // Toggle between game mode and inventory mode
        if(!MWBase::Environment::get().getWindowManager()->isGuiMode())
            MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Inventory);
        else
        {
            MWGui::GuiMode mode = MWBase::Environment::get().getWindowManager()->getMode();
            if(mode == MWGui::GM_Inventory || mode == MWGui::GM_Container)
                MWBase::Environment::get().getWindowManager()->popGuiMode();
        }

        // .. but don't touch any other mode, except container.
    }

    void InputManager::toggleConsole()
    {
        if (MyGUI::InputManager::getInstance ().isModalAny())
            return;

        // Switch to console mode no matter what mode we are currently
        // in, except of course if we are already in console mode
        if (MWBase::Environment::get().getWindowManager()->isGuiMode())
        {
            if (MWBase::Environment::get().getWindowManager()->getMode() == MWGui::GM_Console)
                MWBase::Environment::get().getWindowManager()->popGuiMode();
            else
                MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Console);
        }
        else
            MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Console);
    }

    void InputManager::toggleJournal()
    {
        if (!mControlSwitch["playercontrols"])
            return;
        if (MyGUI::InputManager::getInstance ().isModalAny())
            return;

        if(MWBase::Environment::get().getWindowManager()->getMode() != MWGui::GM_Journal
                && MWBase::Environment::get().getWindowManager ()->getJournalAllowed())
        {
            MWBase::Environment::get().getSoundManager()->playSound ("book open", 1.0, 1.0);
            MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Journal);
        }
        else if(MWBase::Environment::get().getWindowManager()->containsMode(MWGui::GM_Journal))
        {
            MWBase::Environment::get().getWindowManager()->removeGuiMode(MWGui::GM_Journal);
        }
    }

    void InputManager::quickKey (int index)
    {
        if (!mControlSwitch["playercontrols"])
            return;
        MWWorld::Ptr player = MWMechanics::getPlayer();
        if (player.getClass().getNpcStats(player).isWerewolf())
        {
            // Cannot use items or spells while in werewolf form
            MWBase::Environment::get().getWindowManager()->messageBox("#{sWerewolfRefusal}");
            return;
        }

        if (!MWBase::Environment::get().getWindowManager()->isGuiMode())
            MWBase::Environment::get().getWindowManager()->activateQuickKey (index);
    }

    void InputManager::showQuickKeysMenu()
    {
        if (!MWBase::Environment::get().getWindowManager()->isGuiMode ()
                && MWBase::Environment::get().getWorld()->getGlobalFloat ("chargenstate")==-1)
        {
            MWWorld::Ptr player = MWMechanics::getPlayer();
            if (player.getClass().getNpcStats(player).isWerewolf())
            {
                // Cannot use items or spells while in werewolf form
                MWBase::Environment::get().getWindowManager()->messageBox("#{sWerewolfRefusal}");
                return;
            }

            MWBase::Environment::get().getWindowManager()->pushGuiMode (MWGui::GM_QuickKeysMenu);

        }
        else if (MWBase::Environment::get().getWindowManager()->getMode () == MWGui::GM_QuickKeysMenu) {
            while(MyGUI::InputManager::getInstance().isModalAny()) { //Handle any open Modal windows
                MWBase::Environment::get().getWindowManager()->exitCurrentModal();
            }
            MWBase::Environment::get().getWindowManager()->exitCurrentGuiMode(); //And handle the actual main window
        }
    }

    void InputManager::activate()
    {
        if (mControlSwitch["playercontrols"])
            mPlayer->activate();
    }

    void InputManager::toggleAutoMove()
    {
        if (MWBase::Environment::get().getWindowManager()->isGuiMode()) return;

        if (mControlSwitch["playercontrols"])
            mPlayer->setAutoMove (!mPlayer->getAutoMove());
    }

    void InputManager::toggleWalking()
    {
        if (MWBase::Environment::get().getWindowManager()->isGuiMode()) return;
        mAlwaysRunActive = !mAlwaysRunActive;

        Settings::Manager::setBool("always run", "Input", mAlwaysRunActive);
    }

    void InputManager::toggleSneaking()
    {
        if (MWBase::Environment::get().getWindowManager()->isGuiMode()) return;
        mSneaking = !mSneaking;
        mPlayer->setSneak(mSneaking);
    }

    void InputManager::resetIdleTime()
    {
        if (mTimeIdle < 0)
            MWBase::Environment::get().getWorld()->toggleVanityMode(false);
        mTimeIdle = 0.f;
    }

    void InputManager::updateIdleTime(float dt)
    {
        static const float vanityDelay = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>()
                .find("fVanityDelay")->getFloat();
        if (mTimeIdle >= 0.f)
            mTimeIdle += dt;
        if (mTimeIdle > vanityDelay) {
            MWBase::Environment::get().getWorld()->toggleVanityMode(true);
            mTimeIdle = -1.f;
        }
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

        for (int i = 0; i < A_Last; ++i)
        {
            ICS::Control* control;
            bool controlExists = mInputBinder->getChannel(i)->getControlsCount () != 0;
            if (!controlExists)
            {
                control = new ICS::Control(boost::lexical_cast<std::string>(i), false, true, 0, ICS::ICS_MAX, ICS::ICS_MAX);
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
                      ))
            {
                clearAllKeyBindings(control);

                if (defaultKeyBindings.find(i) != defaultKeyBindings.end()
                        && !mInputBinder->isKeyBound(defaultKeyBindings[i]))
                {
                    control->setInitialValue(0.0f);
                    mInputBinder->addKeyBinding(control, defaultKeyBindings[i], ICS::Control::INCREASE);
                }
                else if (defaultMouseButtonBindings.find(i) != defaultMouseButtonBindings.end()
                         && !mInputBinder->isMouseButtonBound(defaultMouseButtonBindings[i]))
                {
                    control->setInitialValue(0.0f);
                    mInputBinder->addMouseButtonBinding (control, defaultMouseButtonBindings[i], ICS::Control::INCREASE);
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
        defaultButtonBindings[A_ToggleSpell] = SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
        //defaultButtonBindings[A_QuickButtonsMenu] = SDL_GetButtonFromScancode(SDL_SCANCODE_F1); // Need to implement, should be ToggleSpell(5) AND Wait(9)
        defaultButtonBindings[A_Sneak] = SDL_CONTROLLER_BUTTON_RIGHTSTICK;
        defaultButtonBindings[A_Jump] = SDL_CONTROLLER_BUTTON_Y;
        defaultButtonBindings[A_Journal] = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
        defaultButtonBindings[A_Rest] = SDL_CONTROLLER_BUTTON_BACK;
        defaultButtonBindings[A_TogglePOV] = SDL_CONTROLLER_BUTTON_LEFTSTICK;
        defaultButtonBindings[A_Inventory] = SDL_CONTROLLER_BUTTON_B;
        defaultButtonBindings[A_GameMenu] = SDL_CONTROLLER_BUTTON_START;
        defaultButtonBindings[A_QuickSave] = SDL_CONTROLLER_BUTTON_GUIDE;
        defaultButtonBindings[A_QuickKey1] = SDL_CONTROLLER_BUTTON_DPAD_UP;
        defaultButtonBindings[A_QuickKey2] = SDL_CONTROLLER_BUTTON_DPAD_LEFT;
        defaultButtonBindings[A_QuickKey3] = SDL_CONTROLLER_BUTTON_DPAD_DOWN;
        defaultButtonBindings[A_QuickKey4] = SDL_CONTROLLER_BUTTON_DPAD_RIGHT;

        std::map<int, int> defaultAxisBindings;
        defaultAxisBindings[A_MoveForwardBackward] = SDL_CONTROLLER_AXIS_LEFTY;
        defaultAxisBindings[A_MoveLeftRight] = SDL_CONTROLLER_AXIS_LEFTX;
        defaultAxisBindings[A_LookUpDown] = SDL_CONTROLLER_AXIS_RIGHTY;
        defaultAxisBindings[A_LookLeftRight] = SDL_CONTROLLER_AXIS_RIGHTX;
        defaultAxisBindings[A_Use] = SDL_CONTROLLER_AXIS_TRIGGERRIGHT;

        for (int i = 0; i < A_Last; i++)
        {
            ICS::Control* control;
            bool controlExists = mInputBinder->getChannel(i)->getControlsCount () != 0;
            if (!controlExists)
            {
                float initial;
                if (defaultButtonBindings.find(i) != defaultButtonBindings.end())
                    initial = 0.0f;
                else initial = 0.5f;
                control = new ICS::Control(boost::lexical_cast<std::string>(i), false, true, initial, ICS::ICS_MAX, ICS::ICS_MAX);
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

                if (defaultButtonBindings.find(i) != defaultButtonBindings.end())
                {
                    control->setInitialValue(0.0f);
                    mInputBinder->addJoystickButtonBinding(control, mFakeDeviceID, defaultButtonBindings[i], ICS::Control::INCREASE);
                }
                else if (defaultAxisBindings.find(i) != defaultAxisBindings.end())
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

        if (mInputBinder->getKeyBinding (c, ICS::Control::INCREASE) != SDL_SCANCODE_UNKNOWN)
            return mInputBinder->scancodeToString (mInputBinder->getKeyBinding (c, ICS::Control::INCREASE));
        else if (mInputBinder->getMouseButtonBinding (c, ICS::Control::INCREASE) != ICS_MAX_DEVICE_BUTTONS)
            return "#{sMouse} " + boost::lexical_cast<std::string>(mInputBinder->getMouseButtonBinding (c, ICS::Control::INCREASE));
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

    std::string InputManager::sdlControllerButtonToString(int button)
    {
        switch(button)
        {
            case SDL_CONTROLLER_BUTTON_A:
                return "A Button";
            case SDL_CONTROLLER_BUTTON_B:
                return "B Button";
            case SDL_CONTROLLER_BUTTON_BACK:
                return "Back Button";
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                return "DPad Down";
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                return "DPad Left";
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                return "DPad Right";
            case SDL_CONTROLLER_BUTTON_DPAD_UP:
                return "DPad Up";
            case SDL_CONTROLLER_BUTTON_GUIDE:
                return "Guide Button";
            case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
                return "Left Shoulder";
            case SDL_CONTROLLER_BUTTON_LEFTSTICK:
                return "Left Stick Button";
            case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
                return "Right Shoulder";
            case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
                return "Right Stick Button";
            case SDL_CONTROLLER_BUTTON_START:
                return "Start Button";
            case SDL_CONTROLLER_BUTTON_X:
                return "X Button";
            case SDL_CONTROLLER_BUTTON_Y:
                return "Y Button";
            default:
                return "Button " + boost::lexical_cast<std::string>(button);
        }
    }
    std::string InputManager::sdlControllerAxisToString(int axis)
    {
        switch(axis)
        {
            case SDL_CONTROLLER_AXIS_LEFTX:
                return "Left Stick X";
            case SDL_CONTROLLER_AXIS_LEFTY:
                return "Left Stick Y";
            case SDL_CONTROLLER_AXIS_RIGHTX:
                return "Right Stick X";
            case SDL_CONTROLLER_AXIS_RIGHTY:
                return "Right Stick Y";
            case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
                return "Left Trigger";
            case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
                return "Right Trigger";
            default:
                return "Axis " + boost::lexical_cast<std::string>(axis);
        }
     }

    std::vector<int> InputManager::getActionKeySorting()
    {
        std::vector<int> ret;
        ret.push_back(A_MoveForward);
        ret.push_back(A_MoveBackward);
        ret.push_back(A_MoveLeft);
        ret.push_back(A_MoveRight);
        ret.push_back(A_TogglePOV);
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

    void InputManager::enableDetectingBindingMode (int action, bool keyboard)
    {
        mDetectingKeyboard = keyboard;
        ICS::Control* c = mInputBinder->getChannel (action)->getAttachedControls ().front().control;
        mInputBinder->enableDetectingBindingState (c, ICS::Control::INCREASE);
    }

    void InputManager::mouseAxisBindingDetected(ICS::InputControlSystem* ICS, ICS::Control* control
        , ICS::InputControlSystem::NamedAxis axis, ICS::Control::ControlChangingDirection direction)
    {
        // we don't want mouse movement bindings
        return;
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
        if(!mDetectingKeyboard)
            return;

        clearAllKeyBindings(control);
        control->setInitialValue(0.0f);
        ICS::DetectingBindingListener::keyBindingDetected (ICS, control, key, direction);
        MWBase::Environment::get().getWindowManager ()->notifyInputActionBound ();
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
    }

    void InputManager::clearAllControllerBindings (ICS::Control* control)
    {
        // right now we don't really need multiple bindings for the same action, so remove all others first
        if (mInputBinder->getJoystickAxisBinding (control, mFakeDeviceID, ICS::Control::INCREASE) != SDL_SCANCODE_UNKNOWN)
            mInputBinder->removeJoystickAxisBinding (mFakeDeviceID, mInputBinder->getJoystickAxisBinding (control, mFakeDeviceID, ICS::Control::INCREASE));
        if (mInputBinder->getJoystickButtonBinding (control, mFakeDeviceID, ICS::Control::INCREASE) != ICS_MAX_DEVICE_BUTTONS)
            mInputBinder->removeJoystickButtonBinding (mFakeDeviceID, mInputBinder->getJoystickButtonBinding (control, mFakeDeviceID, ICS::Control::INCREASE));
    }

    void InputManager::resetToDefaultKeyBindings()
    {
        loadKeyDefaults(true);
    }

    void InputManager::resetToDefaultControllerBindings()
    {
        loadControllerDefaults(true);
    }

    MyGUI::MouseButton InputManager::sdlButtonToMyGUI(Uint8 button)
    {
        //The right button is the second button, according to MyGUI
        if(button == SDL_BUTTON_RIGHT)
            button = SDL_BUTTON_MIDDLE;
        else if(button == SDL_BUTTON_MIDDLE)
            button = SDL_BUTTON_RIGHT;

        //MyGUI's buttons are 0 indexed
        return MyGUI::MouseButton::Enum(button - 1);
    }
}
