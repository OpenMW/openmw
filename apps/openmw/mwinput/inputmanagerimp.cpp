#include "inputmanagerimp.hpp"

#if defined(__APPLE__) && !defined(__LP64__)
#include <Carbon/Carbon.h>
#endif

#include <OgreRoot.h>
#include <OgreRenderWindow.h>

#include <boost/lexical_cast.hpp>

#include <OISInputManager.h>

#include <MyGUI_InputManager.h>
#include <MyGUI_RenderManager.h>
#include <MyGUI_Widget.h>
#include <MyGUI_Button.h>

#include <openengine/ogre/renderer.hpp>

#include "../engine.hpp"

#include "../mwworld/player.hpp"
#include "../mwworld/class.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/soundmanager.hpp"

namespace MWInput
{
    InputManager::InputManager(OEngine::Render::OgreRenderer &ogre,
            MWWorld::Player &player,
            MWBase::WindowManager &windows,
            bool debug,
            OMW::Engine& engine,
            const std::string& userFile, bool userFileExists)
        : mOgre(ogre)
        , mPlayer(player)
        , mWindows(windows)
        , mEngine(engine)
        , mMouseLookEnabled(true)
        , mMouseX(ogre.getWindow()->getWidth ()/2.f)
        , mMouseY(ogre.getWindow()->getHeight ()/2.f)
        , mMouseWheel(0)
        , mDragDrop(false)
        , mGuiCursorEnabled(false)
        , mDebug(debug)
        , mUserFile(userFile)
        , mUserFileExists(userFileExists)
        , mInvertY (Settings::Manager::getBool("invert y axis", "Input"))
        , mCameraSensitivity (Settings::Manager::getFloat("camera sensitivity", "Input"))
        , mUISensitivity (Settings::Manager::getFloat("ui sensitivity", "Input"))
        , mCameraYMultiplier (Settings::Manager::getFloat("camera y multiplier", "Input"))
        , mUIYMultiplier (Settings::Manager::getFloat("ui y multiplier", "Input"))
        , mPreviewPOVDelay(0.f)
        , mTimeIdle(0.f)
        , mOverencumberedMessageDelay(0.f)
        , mAlwaysRunActive(false)
    {
        Ogre::RenderWindow* window = mOgre.getWindow ();
        size_t windowHnd;

        resetIdleTime();

        window->getCustomAttribute("WINDOW", &windowHnd);

        std::ostringstream windowHndStr;
        OIS::ParamList pl;

        windowHndStr << windowHnd;
        pl.insert(std::make_pair(std::string("WINDOW"), windowHndStr.str()));

        // Set non-exclusive mouse and keyboard input if the user requested
        // it.
        if (mDebug)
        {
            #if defined OIS_WIN32_PLATFORM
            pl.insert(std::make_pair(std::string("w32_mouse"),
                std::string("DISCL_FOREGROUND" )));
            pl.insert(std::make_pair(std::string("w32_mouse"),
                std::string("DISCL_NONEXCLUSIVE")));
            pl.insert(std::make_pair(std::string("w32_keyboard"),
                std::string("DISCL_FOREGROUND")));
            pl.insert(std::make_pair(std::string("w32_keyboard"),
                std::string("DISCL_NONEXCLUSIVE")));
            #elif defined OIS_LINUX_PLATFORM
            pl.insert(std::make_pair(std::string("x11_mouse_grab"),
                std::string("false")));
            pl.insert(std::make_pair(std::string("x11_mouse_hide"),
                std::string("false")));
            pl.insert(std::make_pair(std::string("x11_keyboard_grab"),
                std::string("false")));
            #endif
        }
#if defined OIS_LINUX_PLATFORM
        pl.insert(std::make_pair(std::string("XAutoRepeatOn"),
            std::string("true")));
#endif

#if defined(__APPLE__) && !defined(__LP64__)
        // Give the application window focus to receive input events
        ProcessSerialNumber psn = { 0, kCurrentProcess };
        TransformProcessType(&psn, kProcessTransformToForegroundApplication);
        SetFrontProcess(&psn);
#endif

        mInputManager = OIS::InputManager::createInputSystem( pl );

        // Create all devices
        mKeyboard = static_cast<OIS::Keyboard*>(mInputManager->createInputObject
            ( OIS::OISKeyboard, true ));
        mMouse = static_cast<OIS::Mouse*>(mInputManager->createInputObject
            ( OIS::OISMouse, true ));

        mKeyboard->setEventCallback (this);
        mMouse->setEventCallback (this);

        adjustMouseRegion (window->getWidth(), window->getHeight());

        MyGUI::InputManager::getInstance().injectMouseMove(mMouseX, mMouseY, mMouse->getMouseState ().Z.abs);

        std::string file = mUserFileExists ? mUserFile : "";
        mInputCtrl = new ICS::InputControlSystem(file, true, this, NULL, A_Last);

        loadKeyDefaults();

        for (int i = 0; i < A_Last; ++i)
        {
            mInputCtrl->getChannel (i)->addListener (this);
        }

        mControlSwitch["playercontrols"]      = true;
        mControlSwitch["playerfighting"]      = true;
        mControlSwitch["playerjumping"]       = true;
        mControlSwitch["playerlooking"]       = true;
        mControlSwitch["playermagic"]         = true;
        mControlSwitch["playerviewswitch"]    = true;
        mControlSwitch["vanitymode"]          = true;

        changeInputMode(false);
    }

    InputManager::~InputManager()
    {
        mInputCtrl->save (mUserFile);

        delete mInputCtrl;

        mInputManager->destroyInputObject(mKeyboard);
        mInputManager->destroyInputObject(mMouse);
        OIS::InputManager::destroyInputSystem(mInputManager);
    }

    void InputManager::channelChanged(ICS::Channel* channel, float currentValue, float previousValue)
    {
        if (mDragDrop)
            return;

        resetIdleTime ();

        int action = channel->getNumber();
        if (currentValue == 1)
        {
            // trigger action activated
            switch (action)
            {
            case A_GameMenu:
                toggleMainMenu ();
                break;
            case A_Quit:
                exitNow();
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
                if( MWBase::Environment::get().getWindowManager()->isGuiMode()
                    && MWBase::Environment::get().getWindowManager()->getMode() == MWGui::GM_InterMessageBox ) {
                        // Pressing the activation key when a messagebox is prompting for "ok" will activate the ok button
                        MWBase::Environment::get().getWindowManager()->enterPressed();
                    }
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
                mWindows.toggleHud();
                break;
            }
        }
    }

    void InputManager::update(float dt, bool loading)
    {
        // Tell OIS to handle all input events
        mKeyboard->capture();
        mMouse->capture();

        // inject some fake mouse movement to force updating MyGUI's widget states
        // this shouldn't do any harm since we're moving back to the original position afterwards
        MyGUI::InputManager::getInstance().injectMouseMove( int(mMouseX+1), int(mMouseY+1), mMouseWheel);
        MyGUI::InputManager::getInstance().injectMouseMove( int(mMouseX), int(mMouseY), mMouseWheel);

        // update values of channels (as a result of pressed keys)
        if (!loading)
            mInputCtrl->update(dt);
        
        // Update windows/gui as a result of input events
        // For instance this could mean opening a new window/dialog,
        // by doing this after the input events are handled we
        // ensure that window/gui changes appear quickly while
        // avoiding that window/gui changes does not happen in
        // event callbacks (which may crash)
        mWindows.update();

        // Disable movement in Gui mode
        if (mWindows.isGuiMode()) return;


        // Configure player movement according to keyboard input. Actual movement will
        // be done in the physics system.
        if (mControlSwitch["playercontrols"])
        {
            bool triedToMove = false;
            if (actionIsActive(A_MoveLeft))
            {
                triedToMove = true;
                mPlayer.setLeftRight (-1);
            }
            else if (actionIsActive(A_MoveRight))
            {
                triedToMove = true;
                mPlayer.setLeftRight (1);
            }
            else
                mPlayer.setLeftRight (0);

            if (actionIsActive(A_MoveForward))
            {
                triedToMove = true;
                mPlayer.setAutoMove (false);
                mPlayer.setForwardBackward (1);
            }
            else if (actionIsActive(A_MoveBackward))
            {
                triedToMove = true;
                mPlayer.setAutoMove (false);
                mPlayer.setForwardBackward (-1);
            }
            else
                mPlayer.setForwardBackward (0);

            mPlayer.setSneak(actionIsActive(A_Sneak));

            if (actionIsActive(A_Jump) && mControlSwitch["playerjumping"])
            {
                mPlayer.setUpDown (1);
                triedToMove = true;
            }
            else
                mPlayer.setUpDown (0);

            if (mAlwaysRunActive)
                mPlayer.setRunState(!actionIsActive(A_Run));
            else
                mPlayer.setRunState(actionIsActive(A_Run));

            // if player tried to start moving, but can't (due to being overencumbered), display a notification.
            if (triedToMove)
            {
                MWWorld::Ptr player = MWBase::Environment::get().getWorld ()->getPlayer ().getPlayer ();
                mOverencumberedMessageDelay -= dt;
                if (MWWorld::Class::get(player).getEncumbrance(player) >= MWWorld::Class::get(player).getCapacity(player))
                {
                    if (mOverencumberedMessageDelay <= 0)
                    {
                        MWBase::Environment::get().getWindowManager ()->messageBox("#{sNotifyMessage59}");
                        mOverencumberedMessageDelay = 1.0;
                    }
                }
            }

            if (mControlSwitch["playerviewswitch"]) {

                // work around preview mode toggle when pressing Alt+Tab
                if (actionIsActive(A_TogglePOV) && !mKeyboard->isModifierDown (OIS::Keyboard::Alt)) {
                    if (mPreviewPOVDelay <= 0.5 &&
                        (mPreviewPOVDelay += dt) > 0.5)
                    {
                        mPreviewPOVDelay = 1.f;
                        MWBase::Environment::get().getWorld()->togglePreviewMode(true);
                    }
                } else {
                    if (mPreviewPOVDelay > 0.5) {
                        //disable preview mode
                        MWBase::Environment::get().getWorld()->togglePreviewMode(false);
                    } else if (mPreviewPOVDelay > 0.f) {
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

    void InputManager::setDragDrop(bool dragDrop)
    {
        mDragDrop = dragDrop;
    }

    void InputManager::changeInputMode(bool guiMode)
    {
        // Are we in GUI mode now?
        if(guiMode)
        {
            // Disable mouse look
            mMouseLookEnabled = false;

            mWindows.showCrosshair (false);

            // Enable GUI events
            mGuiCursorEnabled = true;
        }
        else
        {
            // Enable mouse look
            mMouseLookEnabled = true;

            mWindows.showCrosshair (false);

            // Disable GUI events
            mGuiCursorEnabled = false;
        }
    }

    void InputManager::processChangedSettings(const Settings::CategorySettingVector& changed)
    {
        bool changeRes = false;
        for (Settings::CategorySettingVector::const_iterator it = changed.begin();
        it != changed.end(); ++it)
        {
            if (it->first == "Video" && (it->second == "resolution x" || it->second == "resolution y"))
                changeRes = true;

            if (it->first == "Input" && it->second == "invert y axis")
                mInvertY = Settings::Manager::getBool("invert y axis", "Input");

            if (it->first == "Input" && it->second == "camera sensitivity")
                mCameraSensitivity = Settings::Manager::getFloat("camera sensitivity", "Input");

            if (it->first == "Input" && it->second == "ui sensitivity")
                mUISensitivity = Settings::Manager::getFloat("ui sensitivity", "Input");

        }

        if (changeRes)
            adjustMouseRegion(Settings::Manager::getInt("resolution x", "Video"), Settings::Manager::getInt("resolution y", "Video"));
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
            mPlayer.setLeftRight(0);
            mPlayer.setForwardBackward(0);
            mPlayer.setAutoMove(false);
            mPlayer.setUpDown(0);
        } else if (sw == "playerjumping" && !value) {
            /// \fixme maybe crouching at this time
            mPlayer.setUpDown(0);
        } else if (sw == "vanitymode") {
            MWBase::Environment::get().getWorld()->allowVanityMode(value);
        } else if (sw == "playerlooking") {
            MWBase::Environment::get().getWorld()->togglePlayerLooking(value);
        }
        mControlSwitch[sw] = value;
    }

    void InputManager::adjustMouseRegion(int width, int height)
    {
        const OIS::MouseState &ms = mMouse->getMouseState();
        ms.width  = width;
        ms.height = height;
    }

    bool InputManager::keyPressed( const OIS::KeyEvent &arg )
    {
        if(arg.key == OIS::KC_RETURN
            && MWBase::Environment::get().getWindowManager()->isGuiMode()
            && MWBase::Environment::get().getWindowManager()->getMode() == MWGui::GM_InterMessageBox )
        {
            // Pressing enter when a messagebox is prompting for "ok" will activate the ok button
            MWBase::Environment::get().getWindowManager()->enterPressed();
        }

        mInputCtrl->keyPressed (arg);
        unsigned int text = arg.text;
#ifdef __APPLE__ // filter \016 symbol for F-keys on OS X
        if ((arg.key >= OIS::KC_F1 && arg.key <= OIS::KC_F10) ||
            (arg.key >= OIS::KC_F11 && arg.key <= OIS::KC_F15)) {
            text = 0;
        }
#endif

        MyGUI::InputManager::getInstance().injectKeyPress(MyGUI::KeyCode::Enum(arg.key), text);

        return true;
    }

    bool InputManager::keyReleased( const OIS::KeyEvent &arg )
    {
        mInputCtrl->keyReleased (arg);

        MyGUI::InputManager::getInstance().injectKeyRelease(MyGUI::KeyCode::Enum(arg.key));

        return true;
    }

    bool InputManager::mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
    {
        mInputCtrl->mousePressed (arg, id);

        MyGUI::InputManager::getInstance().injectMousePress(mMouseX, mMouseY, MyGUI::MouseButton::Enum(id));

        if (MyGUI::InputManager::getInstance ().getMouseFocusWidget () != 0)
        {
            MyGUI::Button* b = MyGUI::InputManager::getInstance ().getMouseFocusWidget ()->castType<MyGUI::Button>(false);
            if (b)
            {
                MWBase::Environment::get().getSoundManager ()->playSound ("Menu Click", 1.f, 1.f);
            }
        }

        return true;
    }

    bool InputManager::mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
    {
        mInputCtrl->mouseReleased (arg, id);

        MyGUI::InputManager::getInstance().injectMouseRelease(mMouseX, mMouseY, MyGUI::MouseButton::Enum(id));

        return true;
    }

    bool InputManager::mouseMoved( const OIS::MouseEvent &arg )
    {
        mInputCtrl->mouseMoved (arg);

        resetIdleTime ();

        if (mGuiCursorEnabled)
        {
            const MyGUI::IntSize& viewSize = MyGUI::RenderManager::getInstance().getViewSize();

            // We keep track of our own mouse position, so that moving the mouse while in
            // game mode does not move the position of the GUI cursor
            mMouseX += float(arg.state.X.rel) * mUISensitivity;
            mMouseY += float(arg.state.Y.rel) * mUISensitivity * mUIYMultiplier;
            mMouseX = std::max(0.f, std::min(mMouseX, float(viewSize.width)));
            mMouseY = std::max(0.f, std::min(mMouseY, float(viewSize.height)));
            mMouseWheel = arg.state.Z.abs;

            MyGUI::InputManager::getInstance().injectMouseMove( int(mMouseX), int(mMouseY), mMouseWheel);
        }

        if (mMouseLookEnabled)
        {
            resetIdleTime();

            float x = arg.state.X.rel * mCameraSensitivity * 0.2;
            float y = arg.state.Y.rel * mCameraSensitivity * 0.2 * (mInvertY ? -1 : 1) * mUIYMultiplier;

            MWBase::World *world = MWBase::Environment::get().getWorld();
            world->rotateObject(world->getPlayer().getPlayer(), -y, 0.f, x, true);

            if (arg.state.Z.rel)
                MWBase::Environment::get().getWorld()->changeVanityModeScale(arg.state.Z.rel);
        }

        return true;
    }

    void InputManager::toggleMainMenu()
    {
        if (MyGUI::InputManager::getInstance ().isModalAny())
            return;

        if (mWindows.isGuiMode () && (mWindows.getMode () == MWGui::GM_MainMenu || mWindows.getMode () == MWGui::GM_Settings))
            mWindows.popGuiMode();
        else if (mWindows.isGuiMode () && mWindows.getMode () == MWGui::GM_Video)
            MWBase::Environment::get().getWorld ()->stopVideo ();
        else
            mWindows.pushGuiMode (MWGui::GM_MainMenu);
    }

    void InputManager::toggleSpell()
    {
        if (mWindows.isGuiMode()) return;

        MWMechanics::DrawState_ state = mPlayer.getDrawState();
        if (state == MWMechanics::DrawState_Weapon || state == MWMechanics::DrawState_Nothing)
        {
            mPlayer.setDrawState(MWMechanics::DrawState_Spell);
            std::cout << "Player has now readied his hands for spellcasting!\n" << std::endl;
        }
        else
        {
            mPlayer.setDrawState(MWMechanics::DrawState_Nothing);
            std::cout << "Player does not have any kind of attack ready now.\n" << std::endl;
        }
    }

    void InputManager::toggleWeapon()
    {
        if (mWindows.isGuiMode()) return;

        MWMechanics::DrawState_ state = mPlayer.getDrawState();
        if (state == MWMechanics::DrawState_Spell || state == MWMechanics::DrawState_Nothing)
        {
            mPlayer.setDrawState(MWMechanics::DrawState_Weapon);
            std::cout << "Player is now drawing his weapon.\n" << std::endl;
        }
        else
        {
            mPlayer.setDrawState(MWMechanics::DrawState_Nothing);
            std::cout << "Player does not have any kind of attack ready now.\n" << std::endl;
        }
    }

    void InputManager::rest()
    {
        if (!mWindows.getRestEnabled () || mWindows.isGuiMode ())
            return;

        /// \todo check if resting is currently allowed (enemies nearby?)
        mWindows.pushGuiMode (MWGui::GM_Rest);
    }

    void InputManager::screenshot()
    {
        mEngine.screenshot();

        std::vector<std::string> empty;
        mWindows.messageBox ("Screenshot saved", empty);
    }

    void InputManager::toggleInventory()
    {
        bool gameMode = !mWindows.isGuiMode();

        // Toggle between game mode and inventory mode
        if(gameMode)
            mWindows.pushGuiMode(MWGui::GM_Inventory);
        else
        {
            MWGui::GuiMode mode = mWindows.getMode();
            if(mode == MWGui::GM_Inventory || mode == MWGui::GM_Container)
                mWindows.popGuiMode();
        }

        // .. but don't touch any other mode, except container.
    }

    void InputManager::toggleConsole()
    {
        if (MyGUI::InputManager::getInstance ().isModalAny())
            return;

        bool gameMode = !mWindows.isGuiMode();

        // Switch to console mode no matter what mode we are currently
        // in, except of course if we are already in console mode
        if (!gameMode)
        {
            if (mWindows.getMode() == MWGui::GM_Console)
                mWindows.popGuiMode();
            else
                mWindows.pushGuiMode(MWGui::GM_Console);
        }
        else
            mWindows.pushGuiMode(MWGui::GM_Console);
    }

    void InputManager::toggleJournal()
    {
        // Toggle between game mode and journal mode
        bool gameMode = !mWindows.isGuiMode();

        if(gameMode)
            mWindows.pushGuiMode(MWGui::GM_Journal);
        else if(mWindows.getMode() == MWGui::GM_Journal)
            mWindows.popGuiMode();
        // .. but don't touch any other mode.
    }

    void InputManager::quickKey (int index)
    {
        mWindows.activateQuickKey (index);
    }

    void InputManager::showQuickKeysMenu()
    {
        if (!mWindows.isGuiMode ())
            mWindows.pushGuiMode (MWGui::GM_QuickKeysMenu);
        else if (mWindows.getMode () == MWGui::GM_QuickKeysMenu)
            mWindows.removeGuiMode (MWGui::GM_QuickKeysMenu);
    }

    void InputManager::activate()
    {
        if (mControlSwitch["playercontrols"])
            mEngine.activate();
    }

    void InputManager::toggleAutoMove()
    {
        if (mWindows.isGuiMode()) return;

        if (mControlSwitch["playercontrols"])
            mPlayer.setAutoMove (!mPlayer.getAutoMove());
    }

    void InputManager::toggleWalking()
    {
        if (mWindows.isGuiMode()) return;
        mAlwaysRunActive = !mAlwaysRunActive;
    }

    // Exit program now button (which is disabled in GUI mode)
    void InputManager::exitNow()
    {
        if(!mWindows.isGuiMode())
            Ogre::Root::getSingleton().queueEndRendering ();
    }

    void InputManager::resetIdleTime()
    {
        if (mTimeIdle < 0) {
            MWBase::Environment::get().getWorld()->toggleVanityMode(false, false);
        }
        mTimeIdle = 0.f;
    }

    void InputManager::updateIdleTime(float dt)
    {
        if (mTimeIdle >= 0.f) {
            mTimeIdle += dt;
        }
        if (mTimeIdle > 30.f) {
            MWBase::Environment::get().getWorld()->toggleVanityMode(true, false);
            mTimeIdle = -1.f;
        }
    }

    bool InputManager::actionIsActive (int id)
    {
        return mInputCtrl->getChannel (id)->getValue () == 1;
    }

    void InputManager::loadKeyDefaults (bool force)
    {
        // using hardcoded key defaults is inevitable, if we want the configuration files to stay valid
        // across different versions of OpenMW (in the case where another input action is added)
        std::map<int, int> defaultKeyBindings;

        defaultKeyBindings[A_Activate] = OIS::KC_SPACE;
        defaultKeyBindings[A_MoveBackward] = OIS::KC_S;
        defaultKeyBindings[A_MoveForward] = OIS::KC_W;
        defaultKeyBindings[A_MoveLeft] = OIS::KC_A;
        defaultKeyBindings[A_MoveRight] = OIS::KC_D;
        defaultKeyBindings[A_ToggleWeapon] = OIS::KC_F;
        defaultKeyBindings[A_ToggleSpell] = OIS::KC_R;
        defaultKeyBindings[A_QuickKeysMenu] = OIS::KC_F1;
        defaultKeyBindings[A_Console] = OIS::KC_F2;
        defaultKeyBindings[A_Run] = OIS::KC_LSHIFT;
        defaultKeyBindings[A_Sneak] = OIS::KC_LCONTROL;
        defaultKeyBindings[A_AutoMove] = OIS::KC_Q;
        defaultKeyBindings[A_Jump] = OIS::KC_E;
        defaultKeyBindings[A_Journal] = OIS::KC_J;
        defaultKeyBindings[A_Rest] = OIS::KC_T;
        defaultKeyBindings[A_GameMenu] = OIS::KC_ESCAPE;
        defaultKeyBindings[A_TogglePOV] = OIS::KC_TAB;
        defaultKeyBindings[A_QuickKey1] = OIS::KC_1;
        defaultKeyBindings[A_QuickKey2] = OIS::KC_2;
        defaultKeyBindings[A_QuickKey3] = OIS::KC_3;
        defaultKeyBindings[A_QuickKey4] = OIS::KC_4;
        defaultKeyBindings[A_QuickKey5] = OIS::KC_5;
        defaultKeyBindings[A_QuickKey6] = OIS::KC_6;
        defaultKeyBindings[A_QuickKey7] = OIS::KC_7;
        defaultKeyBindings[A_QuickKey8] = OIS::KC_8;
        defaultKeyBindings[A_QuickKey9] = OIS::KC_9;
        defaultKeyBindings[A_QuickKey10] = OIS::KC_0;
        defaultKeyBindings[A_Screenshot] = OIS::KC_SYSRQ;
        defaultKeyBindings[A_ToggleHUD] = OIS::KC_F12;
        defaultKeyBindings[A_AlwaysRun] = OIS::KC_Y;

        std::map<int, int> defaultMouseButtonBindings;
        defaultMouseButtonBindings[A_Inventory] = OIS::MB_Right;
        defaultMouseButtonBindings[A_Use] = OIS::MB_Left;

        for (int i = 0; i < A_Last; ++i)
        {
            ICS::Control* control;
            bool controlExists = mInputCtrl->getChannel(i)->getControlsCount () != 0;
            if (!controlExists)
            {
                control = new ICS::Control(boost::lexical_cast<std::string>(i), false, true, 0, ICS::ICS_MAX, ICS::ICS_MAX);
                mInputCtrl->addControl(control);
                control->attachChannel(mInputCtrl->getChannel(i), ICS::Channel::DIRECT);
            }
            else
            {
                control = mInputCtrl->getChannel(i)->getAttachedControls ().front().control;
            }

            if (!controlExists || force ||
                    ( mInputCtrl->getKeyBinding (control, ICS::Control::INCREASE) == OIS::KC_UNASSIGNED
                      && mInputCtrl->getMouseButtonBinding (control, ICS::Control::INCREASE) == ICS_MAX_DEVICE_BUTTONS
                      ))
            {
                clearAllBindings (control);

                if (defaultKeyBindings.find(i) != defaultKeyBindings.end())
                    mInputCtrl->addKeyBinding(control, static_cast<OIS::KeyCode>(defaultKeyBindings[i]), ICS::Control::INCREASE);
                else if (defaultMouseButtonBindings.find(i) != defaultMouseButtonBindings.end())
                    mInputCtrl->addMouseButtonBinding (control, defaultMouseButtonBindings[i], ICS::Control::INCREASE);
            }
        }
    }

    std::string InputManager::getActionDescription (int action)
    {
        std::map<int, std::string> descriptions;

        descriptions[A_Activate] = "sActivate";
        descriptions[A_MoveBackward] = "sBack";
        descriptions[A_MoveForward] = "sForward";
        descriptions[A_MoveLeft] = "sLeft";
        descriptions[A_MoveRight] = "sRight";
        descriptions[A_ToggleWeapon] = "sReady_Weapon";
        descriptions[A_ToggleSpell] = "sReady_Magic";
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

        if (descriptions[action] == "")
            return ""; // not configurable

        return "#{" + descriptions[action] + "}";
    }

    std::string InputManager::getActionBindingName (int action)
    {
        if (mInputCtrl->getChannel (action)->getControlsCount () == 0)
            return "#{sNone}";

        ICS::Control* c = mInputCtrl->getChannel (action)->getAttachedControls ().front().control;

        if (mInputCtrl->getKeyBinding (c, ICS::Control::INCREASE) != OIS::KC_UNASSIGNED)
            return mInputCtrl->keyCodeToString (mInputCtrl->getKeyBinding (c, ICS::Control::INCREASE));
        else if (mInputCtrl->getMouseButtonBinding (c, ICS::Control::INCREASE) != ICS_MAX_DEVICE_BUTTONS)
            return "#{sMouse} " + boost::lexical_cast<std::string>(mInputCtrl->getMouseButtonBinding (c, ICS::Control::INCREASE));
        else
            return "#{sNone}";
    }

    std::vector<int> InputManager::getActionSorting()
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
        ret.push_back(A_ToggleWeapon);
        ret.push_back(A_ToggleSpell);
        ret.push_back(A_AutoMove);
        ret.push_back(A_Jump);
        ret.push_back(A_Inventory);
        ret.push_back(A_Journal);
        ret.push_back(A_Rest);
        ret.push_back(A_Console);
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

    void InputManager::enableDetectingBindingMode (int action)
    {
        ICS::Control* c = mInputCtrl->getChannel (action)->getAttachedControls ().front().control;

        mInputCtrl->enableDetectingBindingState (c, ICS::Control::INCREASE);
    }

    void InputManager::mouseAxisBindingDetected(ICS::InputControlSystem* ICS, ICS::Control* control
        , ICS::InputControlSystem::NamedAxis axis, ICS::Control::ControlChangingDirection direction)
    {
        // we don't want mouse movement bindings
        return;
    }

    void InputManager::keyBindingDetected(ICS::InputControlSystem* ICS, ICS::Control* control
        , OIS::KeyCode key, ICS::Control::ControlChangingDirection direction)
    {
        clearAllBindings(control);
        ICS::DetectingBindingListener::keyBindingDetected (ICS, control, key, direction);
        MWBase::Environment::get().getWindowManager ()->notifyInputActionBound ();
    }

    void InputManager::mouseButtonBindingDetected(ICS::InputControlSystem* ICS, ICS::Control* control
        , unsigned int button, ICS::Control::ControlChangingDirection direction)
    {
        clearAllBindings(control);
        ICS::DetectingBindingListener::mouseButtonBindingDetected (ICS, control, button, direction);
        MWBase::Environment::get().getWindowManager ()->notifyInputActionBound ();
    }

    void InputManager::joystickAxisBindingDetected(ICS::InputControlSystem* ICS, ICS::Control* control
        , int deviceId, int axis, ICS::Control::ControlChangingDirection direction)
    {
        clearAllBindings(control);
        ICS::DetectingBindingListener::joystickAxisBindingDetected (ICS, control, deviceId, axis, direction);
        MWBase::Environment::get().getWindowManager ()->notifyInputActionBound ();
    }

    void InputManager::joystickButtonBindingDetected(ICS::InputControlSystem* ICS, ICS::Control* control
        , int deviceId, unsigned int button, ICS::Control::ControlChangingDirection direction)
    {
        clearAllBindings(control);
        ICS::DetectingBindingListener::joystickButtonBindingDetected (ICS, control, deviceId, button, direction);
        MWBase::Environment::get().getWindowManager ()->notifyInputActionBound ();
    }

    void InputManager::joystickPOVBindingDetected(ICS::InputControlSystem* ICS, ICS::Control* control
        , int deviceId, int pov,ICS:: InputControlSystem::POVAxis axis, ICS::Control::ControlChangingDirection direction)
    {
        clearAllBindings(control);
        ICS::DetectingBindingListener::joystickPOVBindingDetected (ICS, control, deviceId, pov, axis, direction);
        MWBase::Environment::get().getWindowManager ()->notifyInputActionBound ();
    }

    void InputManager::joystickSliderBindingDetected(ICS::InputControlSystem* ICS, ICS::Control* control
        , int deviceId, int slider, ICS::Control::ControlChangingDirection direction)
    {
        clearAllBindings(control);
        ICS::DetectingBindingListener::joystickSliderBindingDetected (ICS, control, deviceId, slider, direction);
        MWBase::Environment::get().getWindowManager ()->notifyInputActionBound ();
    }

    void InputManager::clearAllBindings (ICS::Control* control)
    {
        // right now we don't really need multiple bindings for the same action, so remove all others first
        if (mInputCtrl->getKeyBinding (control, ICS::Control::INCREASE) != OIS::KC_UNASSIGNED)
            mInputCtrl->removeKeyBinding (mInputCtrl->getKeyBinding (control, ICS::Control::INCREASE));
        if (mInputCtrl->getMouseButtonBinding (control, ICS::Control::INCREASE) != ICS_MAX_DEVICE_BUTTONS)
            mInputCtrl->removeMouseButtonBinding (mInputCtrl->getMouseButtonBinding (control, ICS::Control::INCREASE));

        /// \todo add joysticks here once they are added
    }

    void InputManager::resetToDefaultBindings()
    {
        loadKeyDefaults(true);
    }
}
