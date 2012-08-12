#include "inputmanagerimp.hpp"

#include <OgreRoot.h>
#include <OgreRenderWindow.h>

#include <boost/bind.hpp>
#include <boost/filesystem.hpp>

#include <OIS/OISInputManager.h>

#include <MyGUI_InputManager.h>
#include <MyGUI_RenderManager.h>

#include <extern/oics/ICSInputControlSystem.h>

#include <openengine/ogre/renderer.hpp>

#include "../engine.hpp"

#include "../mwworld/player.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

namespace MWInput
{
    InputManager::InputManager(OEngine::Render::OgreRenderer &ogre,
            MWWorld::Player &player,
            MWBase::WindowManager &windows,
            bool debug,
            OMW::Engine& engine,
            const std::string& defaultFile,
            const std::string& userFile, bool userFileExists)
        : mOgre(ogre)
        , mPlayer(player)
        , mWindows(windows)
        , mEngine(engine)
        , mMouseLookEnabled(true)
        , mMouseX(ogre.getWindow()->getWidth ()/2.f)
        , mMouseY(ogre.getWindow()->getHeight ()/2.f)
        , mUserFile(userFile)
        , mDragDrop(false)
    {
        Ogre::RenderWindow* window = ogre.getWindow ();
        size_t windowHnd;

        window->getCustomAttribute("WINDOW", &windowHnd);

        std::ostringstream windowHndStr;
        OIS::ParamList pl;

        windowHndStr << windowHnd;
        pl.insert(std::make_pair(std::string("WINDOW"), windowHndStr.str()));

        // Set non-exclusive mouse and keyboard input if the user requested
        // it.
        if (debug)
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
            pl.insert(std::make_pair(std::string("XAutoRepeatOn"),
                std::string("true")));
            #endif
        }

        #ifdef __APPLE_CC__
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

        std::string configFile;
        if (userFileExists)
            configFile = userFile;
        else
            configFile = defaultFile;

        std::cout << "Loading input configuration: " << configFile << std::endl;

        mInputCtrl = new ICS::InputControlSystem(configFile, true, NULL, NULL, A_LAST);

        for (int i = 0; i < A_LAST; ++i)
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
                activate();
                break;
            case A_Journal:
                toggleJournal ();
                break;
            case A_AutoMove:
                toggleAutoMove ();
                break;
            case A_ToggleSneak:
                /// \todo implement
                break;
            case A_ToggleWalk:
                toggleWalking ();
                break;
            case A_ToggleWeapon:
                toggleWeapon ();
                break;
            case A_ToggleSpell:
                toggleSpell ();
                break;
            }
        }
    }

    void InputManager::update(float dt)
    {
        // Tell OIS to handle all input events
        mKeyboard->capture();
        mMouse->capture();

        // update values of channels (as a result of pressed keys)
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
            if (actionIsActive(A_MoveLeft))
            {
                mPlayer.setAutoMove (false);
                mPlayer.setLeftRight (1);
            }
            else if (actionIsActive(A_MoveRight))
            {
                mPlayer.setAutoMove (false);
                mPlayer.setLeftRight (-1);
            }
            else
                mPlayer.setLeftRight (0);

            if (actionIsActive(A_MoveForward))
            {
                mPlayer.setAutoMove (false);
                mPlayer.setForwardBackward (1);
            }
            else if (actionIsActive(A_MoveBackward))
            {
                mPlayer.setAutoMove (false);
                mPlayer.setForwardBackward (-1);
            }
            else
                mPlayer.setForwardBackward (0);

            if (actionIsActive(A_Jump) && mControlSwitch["playerjumping"])
                mPlayer.setUpDown (1);
            else if (actionIsActive(A_Crouch))
                mPlayer.setUpDown (-1);
            else
                mPlayer.setUpDown (0);
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

            // Enable GUI events
            mGuiCursorEnabled = true;
        }
        else
        {
            // Start mouse-looking again if allowed.
            if (mControlSwitch["playerlooking"]) {
                mMouseLookEnabled = true;
            }

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
        }

        if (changeRes)
            adjustMouseRegion(Settings::Manager::getInt("resolution x", "Video"), Settings::Manager::getInt("resolution y", "Video"));
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
        } else if (sw == "playerlooking") {
            if (value) {
                mMouseLookEnabled = true;
            } else {
                mMouseLookEnabled = false;
            }
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
        mInputCtrl->keyPressed (arg);

        if (mGuiCursorEnabled)
            MyGUI::InputManager::getInstance().injectKeyPress(MyGUI::KeyCode::Enum(arg.key), arg.text);

        return true;
    }

    bool InputManager::keyReleased( const OIS::KeyEvent &arg )
    {
        mInputCtrl->keyReleased (arg);

        if (mGuiCursorEnabled)
            MyGUI::InputManager::getInstance().injectKeyRelease(MyGUI::KeyCode::Enum(arg.key));

        return true;
    }

    bool InputManager::mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
    {
        mInputCtrl->mousePressed (arg, id);

        if (mGuiCursorEnabled)
            MyGUI::InputManager::getInstance().injectMousePress(mMouseX, mMouseY, MyGUI::MouseButton::Enum(id));

        return true;
    }

    bool InputManager::mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
    {
        mInputCtrl->mouseReleased (arg, id);

        if (mGuiCursorEnabled)
            MyGUI::InputManager::getInstance().injectMouseRelease(mMouseX, mMouseY, MyGUI::MouseButton::Enum(id));

        return true;
    }

    bool InputManager::mouseMoved( const OIS::MouseEvent &arg )
    {
        mInputCtrl->mouseMoved (arg);

        if (mGuiCursorEnabled)
        {
            const MyGUI::IntSize& viewSize = MyGUI::RenderManager::getInstance().getViewSize();

            // We keep track of our own mouse position, so that moving the mouse while in
            // game mode does not move the position of the GUI cursor
            mMouseX += arg.state.X.rel;
            mMouseY += arg.state.Y.rel;
            mMouseX = std::max(0, std::min(mMouseX, viewSize.width));
            mMouseY = std::max(0, std::min(mMouseY, viewSize.height));

            MyGUI::InputManager::getInstance().injectMouseMove(mMouseX, mMouseY, arg.state.Z.abs);
        }

        if (mMouseLookEnabled)
        {
            float x = arg.state.X.rel * 0.2;
            float y = arg.state.Y.rel * 0.2;

            MWBase::World *world = MWBase::Environment::get().getWorld();
            world->rotateObject(world->getPlayer().getPlayer(), -y, 0.f, x, true);
        }

        return true;
    }

    void InputManager::toggleMainMenu()
    {
        if (mWindows.isGuiMode () && (mWindows.getMode () == MWGui::GM_MainMenu || mWindows.getMode () == MWGui::GM_Settings))
            mWindows.popGuiMode();
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
        else if(mWindows.getMode() == MWGui::GM_Inventory)
            mWindows.popGuiMode();

        // .. but don't touch any other mode.
    }

    void InputManager::toggleConsole()
    {
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

    void InputManager::activate()
    {
        mEngine.activate();
    }

    void InputManager::toggleAutoMove()
    {
        if (mWindows.isGuiMode()) return;
        mPlayer.setAutoMove (!mPlayer.getAutoMove());
    }

    void InputManager::toggleWalking()
    {
        if (mWindows.isGuiMode()) return;
        mPlayer.toggleRunning();
    }

    // Exit program now button (which is disabled in GUI mode)
    void InputManager::exitNow()
    {
        if(!mWindows.isGuiMode())
            Ogre::Root::getSingleton().queueEndRendering ();
    }

    bool InputManager::actionIsActive (int id)
    {
        return mInputCtrl->getChannel (id)->getValue () == 1;
    }

}
