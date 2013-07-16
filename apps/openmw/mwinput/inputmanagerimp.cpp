#include "inputmanagerimp.hpp"

#include <OgreRoot.h>
#include <OgreRenderWindow.h>

#include <boost/lexical_cast.hpp>

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
#include "../mwgui/bookwindow.hpp"
#include "../mwmechanics/creaturestats.hpp"

using namespace ICS;

namespace
{
    std::vector<unsigned long> utf8ToUnicode(const std::string& utf8)
    {
        std::vector<unsigned long> unicode;
        size_t i = 0;
        while (i < utf8.size())
        {
            unsigned long uni;
            size_t todo;
            unsigned char ch = utf8[i++];
            if (ch <= 0x7F)
            {
                uni = ch;
                todo = 0;
            }
            else if (ch <= 0xBF)
            {
                throw std::logic_error("not a UTF-8 string");
            }
            else if (ch <= 0xDF)
            {
                uni = ch&0x1F;
                todo = 1;
            }
            else if (ch <= 0xEF)
            {
                uni = ch&0x0F;
                todo = 2;
            }
            else if (ch <= 0xF7)
            {
                uni = ch&0x07;
                todo = 3;
            }
            else
            {
                throw std::logic_error("not a UTF-8 string");
            }
            for (size_t j = 0; j < todo; ++j)
            {
                if (i == utf8.size())
                    throw std::logic_error("not a UTF-8 string");
                unsigned char ch = utf8[i++];
                if (ch < 0x80 || ch > 0xBF)
                    throw std::logic_error("not a UTF-8 string");
                uni <<= 6;
                uni += ch & 0x3F;
            }
            if (uni >= 0xD800 && uni <= 0xDFFF)
                throw std::logic_error("not a UTF-8 string");
            if (uni > 0x10FFFF)
                throw std::logic_error("not a UTF-8 string");
            unicode.push_back(uni);
        }
        return unicode;
    }
}

namespace MWInput
{
    InputManager::InputManager(OEngine::Render::OgreRenderer &ogre,
            MWWorld::Player& player,
            MWBase::WindowManager &windows,
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
        , mUserFile(userFile)
        , mUserFileExists(userFileExists)
        , mInvertY (Settings::Manager::getBool("invert y axis", "Input"))
        , mCameraSensitivity (Settings::Manager::getFloat("camera sensitivity", "Input"))
        , mUISensitivity (Settings::Manager::getFloat("ui sensitivity", "Input"))
        , mCameraYMultiplier (Settings::Manager::getFloat("camera y multiplier", "Input"))
        , mPreviewPOVDelay(0.f)
        , mTimeIdle(0.f)
        , mOverencumberedMessageDelay(0.f)
        , mAlwaysRunActive(false)
    {

        Ogre::RenderWindow* window = ogre.getWindow ();

        mInputManager = new SFO::InputWrapper(mOgre.getSDLWindow(), mOgre.getWindow());
        mInputManager->setMouseEventCallback (this);
        mInputManager->setKeyboardEventCallback (this);
        mInputManager->setWindowEventCallback(this);

        std::string file = userFileExists ? userFile : "";
        mInputBinder = new ICS::InputControlSystem(file, true, this, NULL, A_Last);

        adjustMouseRegion (window->getWidth(), window->getHeight());

        MyGUI::InputManager::getInstance().injectMouseMove(mMouseX, mMouseY, 0);

        loadKeyDefaults();

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

        changeInputMode(false);
    }

    InputManager::~InputManager()
    {
        mInputBinder->save (mUserFile);

        delete mInputBinder;

        delete mInputManager;
    }

    void InputManager::channelChanged(ICS::Channel* channel, float currentValue, float previousValue)
    {
        if (mDragDrop)
            return;

        resetIdleTime ();

        int action = channel->getNumber();

        if (action == A_Use)
        {
            MWWorld::Class::get(mPlayer.getPlayer()).getCreatureStats(mPlayer.getPlayer()).setAttackingOrSpell(currentValue);
            if (currentValue == 1)
            {
                int type = MWMechanics::CreatureStats::AT_Chop;
                bool forward = (mInputBinder->getChannel(A_MoveForward)->getValue() > 0
                                || mInputBinder->getChannel(A_MoveBackward)->getValue() > 0);
                bool side = (mInputBinder->getChannel(A_MoveLeft)->getValue() > 0
                             || mInputBinder->getChannel(A_MoveRight)->getValue() > 0);
                if (side && !forward)
                    type = MWMechanics::CreatureStats::AT_Slash;
                if (forward && !side)
                    type = MWMechanics::CreatureStats::AT_Thrust;

                MWWorld::Class::get(mPlayer.getPlayer()).getCreatureStats(mPlayer.getPlayer()).setAttackType(type);
            }
        }

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

                if (mWindows.getMode() == MWGui::GM_Container) {
                    toggleContainer (); 
                } else if (MWBase::Environment::get().getWindowManager()->isGuiMode()) {
                    MWBase::Environment::get().getWindowManager()->enterPressed();
                } else {
                    activate();
                }
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
            case A_Use:
                if (!MWBase::Environment::get().getWindowManager()->isGuiMode())
                    mPlayer.use();
                break;
            }
        }
    }

    void InputManager::update(float dt, bool loading)
    {
        // Tell OIS to handle all input events
        mInputManager->capture();
        // inject some fake mouse movement to force updating MyGUI's widget states
        // this shouldn't do any harm since we're moving back to the original position afterwards
        MyGUI::InputManager::getInstance().injectMouseMove( int(mMouseX+1), int(mMouseY+1), mMouseWheel);
        MyGUI::InputManager::getInstance().injectMouseMove( int(mMouseX), int(mMouseY), mMouseWheel);

        // update values of channels (as a result of pressed keys)
        if (!loading)
            mInputBinder->update(dt);

        // Update windows/gui as a result of input events
        // For instance this could mean opening a new window/dialog,
        // by doing this after the input events are handled we
        // ensure that window/gui changes appear quickly while
        // avoiding that window/gui changes does not happen in
        // event callbacks (which may crash)
        mWindows.update();

        bool main_menu = mWindows.containsMode(MWGui::GM_MainMenu);

        bool was_relative = mInputManager->getMouseRelative();
        bool is_relative = !mWindows.isGuiMode();

        // don't keep the pointer away from the window edge in gui mode
        // stop using raw mouse motions and switch to system cursor movements
        mInputManager->setMouseRelative(is_relative);

        //we let the mouse escape in the main menu
        mInputManager->setGrabPointer(!main_menu);

        //we switched to non-relative mode, move our cursor to where the in-game
        //cursor is
        if( !is_relative && was_relative != is_relative )
        {
            mInputManager->warpMouse(mMouseX, mMouseY);
        }

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

            else if(mPlayer.getAutoMove())
            {
                triedToMove = true;
                mPlayer.setForwardBackward (1);
            }

            mPlayer.setSneak(actionIsActive(A_Sneak));

            if (actionIsActive(A_Jump) && mControlSwitch["playerjumping"])
            {
                mPlayer.setUpDown (1);
                triedToMove = true;
            }

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
                    mPlayer.setAutoMove (false);
                    if (mOverencumberedMessageDelay <= 0)
                    {
                        MWBase::Environment::get().getWindowManager ()->messageBox("#{sNotifyMessage59}");
                        mOverencumberedMessageDelay = 1.0;
                    }
                }
            }

            if (mControlSwitch["playerviewswitch"]) {

                // work around preview mode toggle when pressing Alt+Tab
                if (actionIsActive(A_TogglePOV) && !mInputManager->isModifierHeld(KMOD_ALT)) {
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
        mGuiCursorEnabled = guiMode;
        mMouseLookEnabled = !guiMode;
        if (guiMode)
            mWindows.showCrosshair(false);
        mWindows.setCursorVisible(guiMode);
        // if not in gui mode, the camera decides whether to show crosshair or not.
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
        mInputBinder->adjustMouseRegion(width, height);
    }

    bool InputManager::keyPressed( const SDL_KeyboardEvent &arg )
    {
        mInputBinder->keyPressed (arg);

        if(arg.keysym.sym == SDLK_RETURN
            && MWBase::Environment::get().getWindowManager()->isGuiMode())
        {
            // Pressing enter when a messagebox is prompting for "ok" will activate the ok button
            MWBase::Environment::get().getWindowManager()->enterPressed();
        }

        OIS::KeyCode kc = mInputManager->sdl2OISKeyCode(arg.keysym.sym);

        if (kc != OIS::KC_UNASSIGNED)
            MyGUI::InputManager::getInstance().injectKeyPress(MyGUI::KeyCode::Enum(kc), 0);
        return true;
    }

    void InputManager::textInput(const SDL_TextInputEvent &arg)
    {
        const char* text = &arg.text[0];
        std::vector<unsigned long> unicode = utf8ToUnicode(std::string(text));
        for (std::vector<unsigned long>::iterator it = unicode.begin(); it != unicode.end(); ++it)
            MyGUI::InputManager::getInstance().injectKeyPress(MyGUI::KeyCode::None, *it);
    }

    bool InputManager::keyReleased(const SDL_KeyboardEvent &arg )
    {
        mInputBinder->keyReleased (arg);

        OIS::KeyCode kc = mInputManager->sdl2OISKeyCode(arg.keysym.sym);

        MyGUI::InputManager::getInstance().injectKeyRelease(MyGUI::KeyCode::Enum(kc));

        return true;
    }

    bool InputManager::mousePressed( const SDL_MouseButtonEvent &arg, Uint8 id )
    {
        mInputBinder->mousePressed (arg, id);

        if (id != SDL_BUTTON_LEFT && id != SDL_BUTTON_RIGHT)
            return true; // MyGUI has no use for these events

        MyGUI::InputManager::getInstance().injectMousePress(mMouseX, mMouseY, sdlButtonToMyGUI(id));
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

    bool InputManager::mouseReleased( const SDL_MouseButtonEvent &arg, Uint8 id )
    {
        mInputBinder->mouseReleased (arg, id);

        MyGUI::InputManager::getInstance().injectMouseRelease(mMouseX, mMouseY, sdlButtonToMyGUI(id));

        return true;
    }

    bool InputManager::mouseMoved(const SFO::MouseMotionEvent &arg )
    {
        mInputBinder->mouseMoved (arg);

        resetIdleTime ();

        if (mGuiCursorEnabled)
        {
            const MyGUI::IntSize& viewSize = MyGUI::RenderManager::getInstance().getViewSize();

            // We keep track of our own mouse position, so that moving the mouse while in
            // game mode does not move the position of the GUI cursor
            mMouseX = arg.x;
            mMouseY = arg.y;

            mMouseX = std::max(0.f, std::min(mMouseX, float(viewSize.width)));
            mMouseY = std::max(0.f, std::min(mMouseY, float(viewSize.height)));

            mMouseWheel = int(arg.z);

            MyGUI::InputManager::getInstance().injectMouseMove( int(mMouseX), int(mMouseY), mMouseWheel);

            //if the player is reading a book and flicking the mouse wheel
            if (MWBase::Environment::get().getWindowManager()->getMode() == MWGui::GM_Book && arg.zrel)
            {
                if (arg.zrel < 0)
                    MWBase::Environment::get().getWindowManager()->getBookWindow()->nextPage();
                else
                    MWBase::Environment::get().getWindowManager()->getBookWindow()->prevPage();
            }
        }

        if (mMouseLookEnabled)
        {
            resetIdleTime();

            double x = arg.xrel * mCameraSensitivity * (1.0f/256.f);
            double y = arg.yrel * mCameraSensitivity * (1.0f/256.f) * (mInvertY ? -1 : 1) * mCameraYMultiplier;
            float scale = MWBase::Environment::get().getFrameDuration();
            if(scale <= 0.0f) scale = 1.0f;

            float rot[3];
            rot[0] = -y;
            rot[1] = 0.0f;
            rot[2] = x;
            
            // Only actually turn player when we're not in vanity mode 
            if(!MWBase::Environment::get().getWorld()->vanityRotateCamera(rot))
            {
                mPlayer.yaw(x/scale);
                mPlayer.pitch(-y/scale);
            }

            if (arg.zrel)
                MWBase::Environment::get().getWorld()->changeVanityModeScale(arg.zrel);
        }

        return true;
    }

    bool InputManager::windowFocusChange(bool have_focus)
    {
        return true;
    }

    bool InputManager::windowVisibilityChange(bool visible)
    {
            //TODO: Pause game?
        return true;
    }

    void InputManager::toggleMainMenu()
    {
        if (MyGUI::InputManager::getInstance ().isModalAny())
            return;

        if (mWindows.isGuiMode () && mWindows.getMode () == MWGui::GM_Video)
            MWBase::Environment::get().getWorld ()->stopVideo ();
        else if (mWindows.containsMode(MWGui::GM_MainMenu))
            mWindows.popGuiMode();
        else
            mWindows.pushGuiMode (MWGui::GM_MainMenu);
    }

    void InputManager::toggleSpell()
    {
        if (mWindows.isGuiMode()) return;

        MWMechanics::DrawState_ state = mPlayer.getDrawState();
        if (state == MWMechanics::DrawState_Weapon || state == MWMechanics::DrawState_Nothing)
            mPlayer.setDrawState(MWMechanics::DrawState_Spell);
        else
            mPlayer.setDrawState(MWMechanics::DrawState_Nothing);
    }

    void InputManager::toggleWeapon()
    {
        if (mWindows.isGuiMode()) return;

        MWMechanics::DrawState_ state = mPlayer.getDrawState();
        if (state == MWMechanics::DrawState_Spell || state == MWMechanics::DrawState_Nothing)
            mPlayer.setDrawState(MWMechanics::DrawState_Weapon);
        else
            mPlayer.setDrawState(MWMechanics::DrawState_Nothing);
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
        if (MyGUI::InputManager::getInstance ().isModalAny())
            return;

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

    void InputManager::toggleContainer()
    {
        if (MyGUI::InputManager::getInstance ().isModalAny())
            return;

        bool gameMode = !mWindows.isGuiMode();

        if(!gameMode)
        {
            if (mWindows.getMode() == MWGui::GM_Container)
                mWindows.popGuiMode();
            else
                mWindows.pushGuiMode(MWGui::GM_Container);
        }

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
        if (MyGUI::InputManager::getInstance ().isModalAny())
            return;

        // Toggle between game mode and journal mode
        bool gameMode = !mWindows.isGuiMode();

        if(gameMode && MWBase::Environment::get().getWindowManager ()->getJournalAllowed())
        {
            MWBase::Environment::get().getSoundManager()->playSound ("book open", 1.0, 1.0);
            mWindows.pushGuiMode(MWGui::GM_Journal);
        }
        else if(mWindows.getMode() == MWGui::GM_Journal)
        {
            MWBase::Environment::get().getSoundManager()->playSound ("book close", 1.0, 1.0);
            mWindows.popGuiMode();
        }
        // .. but don't touch any other mode.
    }

    void InputManager::quickKey (int index)
    {
        if (!mWindows.isGuiMode())
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
        if (mTimeIdle < 0)
            MWBase::Environment::get().getWorld()->toggleVanityMode(false);
        mTimeIdle = 0.f;
    }

    void InputManager::updateIdleTime(float dt)
    {
        if (mTimeIdle >= 0.f)
            mTimeIdle += dt;
        if (mTimeIdle > 30.f) {
            MWBase::Environment::get().getWorld()->toggleVanityMode(true);
            mTimeIdle = -1.f;
        }
    }

    bool InputManager::actionIsActive (int id)
    {
        return mInputBinder->getChannel (id)->getValue () == 1;
    }

    void InputManager::loadKeyDefaults (bool force)
    {
        // using hardcoded key defaults is inevitable, if we want the configuration files to stay valid
        // across different versions of OpenMW (in the case where another input action is added)
        std::map<int, int> defaultKeyBindings;

        defaultKeyBindings[A_Activate] = SDLK_SPACE;
        defaultKeyBindings[A_MoveBackward] = SDLK_s;
        defaultKeyBindings[A_MoveForward] = SDLK_w;
        defaultKeyBindings[A_MoveLeft] = SDLK_a;
        defaultKeyBindings[A_MoveRight] = SDLK_d;
        defaultKeyBindings[A_ToggleWeapon] = SDLK_f;
        defaultKeyBindings[A_ToggleSpell] = SDLK_r;
        defaultKeyBindings[A_QuickKeysMenu] = SDLK_F1;
        defaultKeyBindings[A_Console] = SDLK_F2;
        defaultKeyBindings[A_Run] = SDLK_LSHIFT;
        defaultKeyBindings[A_Sneak] = SDLK_LCTRL;
        defaultKeyBindings[A_AutoMove] = SDLK_q;
        defaultKeyBindings[A_Jump] = SDLK_e;
        defaultKeyBindings[A_Journal] = SDLK_j;
        defaultKeyBindings[A_Rest] = SDLK_t;
        defaultKeyBindings[A_GameMenu] = SDLK_ESCAPE;
        defaultKeyBindings[A_TogglePOV] = SDLK_TAB;
        defaultKeyBindings[A_QuickKey1] = SDLK_1;
        defaultKeyBindings[A_QuickKey2] = SDLK_2;
        defaultKeyBindings[A_QuickKey3] = SDLK_3;
        defaultKeyBindings[A_QuickKey4] = SDLK_4;
        defaultKeyBindings[A_QuickKey5] = SDLK_5;
        defaultKeyBindings[A_QuickKey6] = SDLK_6;
        defaultKeyBindings[A_QuickKey7] = SDLK_7;
        defaultKeyBindings[A_QuickKey8] = SDLK_8;
        defaultKeyBindings[A_QuickKey9] = SDLK_9;
        defaultKeyBindings[A_QuickKey10] = SDLK_0;
        defaultKeyBindings[A_Screenshot] = SDLK_PRINTSCREEN;
        defaultKeyBindings[A_ToggleHUD] = SDLK_F12;
        defaultKeyBindings[A_AlwaysRun] = SDLK_y;

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
                    ( mInputBinder->getKeyBinding (control, ICS::Control::INCREASE) == SDLK_UNKNOWN
                      && mInputBinder->getMouseButtonBinding (control, ICS::Control::INCREASE) == ICS_MAX_DEVICE_BUTTONS
                      ))
            {
                clearAllBindings (control);

                if (defaultKeyBindings.find(i) != defaultKeyBindings.end())
                    mInputBinder->addKeyBinding(control, static_cast<SDL_Keycode>(defaultKeyBindings[i]), ICS::Control::INCREASE);
                else if (defaultMouseButtonBindings.find(i) != defaultMouseButtonBindings.end())
                    mInputBinder->addMouseButtonBinding (control, defaultMouseButtonBindings[i], ICS::Control::INCREASE);
            }
        }
    }

    std::string InputManager::getActionDescription (int action)
    {
        std::map<int, std::string> descriptions;

        descriptions[A_Use] = "sUse";
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
        if (mInputBinder->getChannel (action)->getControlsCount () == 0)
            return "#{sNone}";

        ICS::Control* c = mInputBinder->getChannel (action)->getAttachedControls ().front().control;

        if (mInputBinder->getKeyBinding (c, ICS::Control::INCREASE) != SDLK_UNKNOWN)
            return mInputBinder->keyCodeToString (mInputBinder->getKeyBinding (c, ICS::Control::INCREASE));
        else if (mInputBinder->getMouseButtonBinding (c, ICS::Control::INCREASE) != ICS_MAX_DEVICE_BUTTONS)
            return "#{sMouse} " + boost::lexical_cast<std::string>(mInputBinder->getMouseButtonBinding (c, ICS::Control::INCREASE));
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
        ret.push_back(A_Use);
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
        , SDL_Keycode key, ICS::Control::ControlChangingDirection direction)
    {
        //Disallow binding escape key
        if(key==SDLK_ESCAPE)
            return;

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
        if (mInputBinder->getKeyBinding (control, ICS::Control::INCREASE) != SDLK_UNKNOWN)
            mInputBinder->removeKeyBinding (mInputBinder->getKeyBinding (control, ICS::Control::INCREASE));
        if (mInputBinder->getMouseButtonBinding (control, ICS::Control::INCREASE) != ICS_MAX_DEVICE_BUTTONS)
            mInputBinder->removeMouseButtonBinding (mInputBinder->getMouseButtonBinding (control, ICS::Control::INCREASE));

        /// \todo add joysticks here once they are added
    }

    void InputManager::resetToDefaultBindings()
    {
        loadKeyDefaults(true);
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
