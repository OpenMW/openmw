#include "inputmanagerimp.hpp"

#include <OgreRoot.h>
#include <OgreRenderWindow.h>

#include <boost/lexical_cast.hpp>

#include <MyGUI_InputManager.h>
#include <MyGUI_RenderManager.h>
#include <MyGUI_Widget.h>
#include <MyGUI_Button.h>
#include <MyGUI_EditBox.h>

#include <openengine/ogre/renderer.hpp>

#include "../engine.hpp"

#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/statemanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "../mwworld/player.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwmechanics/npcstats.hpp"

#include "../mwdialogue/dialoguemanagerimp.hpp"

#include "../mwgui/windowbase.hpp"

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
            OMW::Engine& engine,
            const std::string& userFile, bool userFileExists, bool grab)
        : mOgre(ogre)
        , mPlayer(NULL)
        , mEngine(engine)
        , mMouseLookEnabled(false)
        , mMouseX(ogre.getWindow()->getWidth ()/2.f)
        , mMouseY(ogre.getWindow()->getHeight ()/2.f)
        , mMouseWheel(0)
        , mDragDrop(false)
        , mGuiCursorEnabled(true)
        , mUserFile(userFile)
        , mUserFileExists(userFileExists)
        , mInvertY (Settings::Manager::getBool("invert y axis", "Input"))
        , mCameraSensitivity (Settings::Manager::getFloat("camera sensitivity", "Input"))
        , mUISensitivity (Settings::Manager::getFloat("ui sensitivity", "Input"))
        , mCameraYMultiplier (Settings::Manager::getFloat("camera y multiplier", "Input"))
        , mGrabCursor (Settings::Manager::getBool("grab cursor", "Input"))
        , mPreviewPOVDelay(0.f)
        , mTimeIdle(0.f)
        , mOverencumberedMessageDelay(0.f)
        , mAlwaysRunActive(Settings::Manager::getBool("always run", "Input"))
        , mAttemptJump(false)
        , mControlsDisabled(false)
    {

        Ogre::RenderWindow* window = ogre.getWindow ();

        mInputManager = new SFO::InputWrapper(mOgre.getSDLWindow(), mOgre.getWindow(), grab);
        mInputManager->setMouseEventCallback (this);
        mInputManager->setKeyboardEventCallback (this);
        mInputManager->setWindowEventCallback(this);

        std::string file = userFileExists ? userFile : "";
        mInputBinder = new ICS::InputControlSystem(file, true, this, NULL, A_Last);

        adjustMouseRegion (window->getWidth(), window->getHeight());

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

        if (action == A_Use)
        {
            mPlayer->getPlayer().getClass().getCreatureStats(mPlayer->getPlayer()).setAttackingOrSpell(currentValue);
        }

        if (action == A_Jump)
        {
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
            case A_QuickSave:
                quickSave();
                break;
            case A_QuickLoad:
                quickLoad();
                break;
            }
        }
    }

    void InputManager::update(float dt, bool disableControls, bool disableEvents)
    {
        mControlsDisabled = disableControls;

        mInputManager->setMouseVisible(MWBase::Environment::get().getWindowManager()->getCursorVisible());

        mInputManager->capture(disableEvents);
        // inject some fake mouse movement to force updating MyGUI's widget states
        MyGUI::InputManager::getInstance().injectMouseMove( int(mMouseX), int(mMouseY), mMouseWheel);

        if (mControlsDisabled)
            return;

        // update values of channels (as a result of pressed keys)
        mInputBinder->update(dt);

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
            mInputManager->warpMouse(mMouseX, mMouseY);
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

                mPlayer->setSneak(actionIsActive(A_Sneak));

                if (mAttemptJump && mControlSwitch["playerjumping"])
                {
                    mPlayer->setUpDown (1);
                    triedToMove = true;
                }

                if (mAlwaysRunActive)
                    mPlayer->setRunState(!actionIsActive(A_Run));
                else
                    mPlayer->setRunState(actionIsActive(A_Run));

                // if player tried to start moving, but can't (due to being overencumbered), display a notification.
                if (triedToMove)
                {
                    MWWorld::Ptr player = MWBase::Environment::get().getWorld ()->getPlayerPtr();
                    mOverencumberedMessageDelay -= dt;
                    if (player.getClass().getEncumbrance(player) >= player.getClass().getCapacity(player))
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

                    // work around preview mode toggle when pressing Alt+Tab
                    if (actionIsActive(A_TogglePOV) && !mInputManager->isModifierHeld(SDL_Keymod(KMOD_ALT))) {
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

    void InputManager::adjustMouseRegion(int width, int height)
    {
        mInputBinder->adjustMouseRegion(width, height);
    }

    void InputManager::keyPressed( const SDL_KeyboardEvent &arg )
    {
        // Cut, copy & paste
        MyGUI::Widget* focus = MyGUI::InputManager::getInstance().getKeyFocusWidget();
        if (focus)
        {
            MyGUI::EditBox* edit = focus->castType<MyGUI::EditBox>(false);
            if (edit && !edit->getEditReadOnly())
            {
                if (arg.keysym.sym == SDLK_v && (arg.keysym.mod & SDL_Keymod(KMOD_CTRL)))
                {
                    char* text = SDL_GetClipboardText();

                    if (text)
                    {
                        edit->insertText(MyGUI::UString(text), edit->getTextCursor());
                        SDL_free(text);
                    }
                }
                if (arg.keysym.sym == SDLK_x && (arg.keysym.mod & SDL_Keymod(KMOD_CTRL)))
                {
                    // Discard color codes and other escape characters
                    std::string text = MyGUI::TextIterator::getOnlyText(edit->getTextSelection());
                    if (text.length())
                    {
                        SDL_SetClipboardText(text.c_str());
                        edit->deleteTextSelection();
                    }
                }
            }
            if (edit && !edit->getEditStatic())
            {
                if (arg.keysym.sym == SDLK_c && (arg.keysym.mod & SDL_Keymod(KMOD_CTRL)))
                {
                    // Discard color codes and other escape characters
                    std::string text = MyGUI::TextIterator::getOnlyText(edit->getTextSelection());
                    if (text.length())
                        SDL_SetClipboardText(text.c_str());
                }
            }
        }

        OIS::KeyCode kc = mInputManager->sdl2OISKeyCode(arg.keysym.sym);

        if (kc != OIS::KC_UNASSIGNED)
        {
            bool guiFocus = MyGUI::InputManager::getInstance().injectKeyPress(MyGUI::KeyCode::Enum(kc), 0);
            setPlayerControlsEnabled(!guiFocus);
        }
        if (!mControlsDisabled)
            mInputBinder->keyPressed (arg);

        // Clear MyGUI's clipboard, so it doesn't interfere with our own clipboard implementation.
        // We do not use MyGUI's clipboard manager because it doesn't support system clipboard integration with SDL.
        MyGUI::ClipboardManager::getInstance().clearClipboardData("Text");
    }

    void InputManager::textInput(const SDL_TextInputEvent &arg)
    {
        const char* text = &arg.text[0];
        std::vector<unsigned long> unicode = utf8ToUnicode(std::string(text));
        for (std::vector<unsigned long>::iterator it = unicode.begin(); it != unicode.end(); ++it)
            MyGUI::InputManager::getInstance().injectKeyPress(MyGUI::KeyCode::None, *it);
    }

    void InputManager::keyReleased(const SDL_KeyboardEvent &arg )
    {
        OIS::KeyCode kc = mInputManager->sdl2OISKeyCode(arg.keysym.sym);

        setPlayerControlsEnabled(!MyGUI::InputManager::getInstance().injectKeyRelease(MyGUI::KeyCode::Enum(kc)));
        mInputBinder->keyReleased (arg);
    }

    void InputManager::mousePressed( const SDL_MouseButtonEvent &arg, Uint8 id )
    {
        bool guiMode = false;

        if (id == SDL_BUTTON_LEFT || id == SDL_BUTTON_RIGHT) // MyGUI only uses these mouse events
        {
            guiMode = MWBase::Environment::get().getWindowManager()->isGuiMode();
            guiMode = MyGUI::InputManager::getInstance().injectMousePress(mMouseX, mMouseY, sdlButtonToMyGUI(id)) && guiMode;
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
        mInputBinder->mousePressed (arg, id);


    }

    void InputManager::mouseReleased( const SDL_MouseButtonEvent &arg, Uint8 id )
    {      

        if(mInputBinder->detectingBindingState())
        {
            mInputBinder->mouseReleased (arg, id);
        } else {
            bool guiMode = MWBase::Environment::get().getWindowManager()->isGuiMode();
            guiMode = MyGUI::InputManager::getInstance().injectMouseRelease(mMouseX, mMouseY, sdlButtonToMyGUI(id)) && guiMode;

            if(mInputBinder->detectingBindingState()) return; // don't allow same mouseup to bind as initiated bind

            setPlayerControlsEnabled(!guiMode);
            mInputBinder->mouseReleased (arg, id);
        }
    }

    void InputManager::mouseMoved(const SFO::MouseMotionEvent &arg )
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
        }

        if (mMouseLookEnabled)
        {
            resetIdleTime();

            double x = arg.xrel * mCameraSensitivity * (1.0f/256.f);
            double y = arg.yrel * mCameraSensitivity * (1.0f/256.f) * (mInvertY ? -1 : 1) * mCameraYMultiplier;

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

            if (arg.zrel && mControlSwitch["playerviewswitch"]) //Check to make sure you are allowed to zoomout and there is a change
            {
                MWBase::Environment::get().getWorld()->changeVanityModeScale(arg.zrel);
                MWBase::Environment::get().getWorld()->setCameraDistance(arg.zrel, true, true);
            }
        }
    }

    void InputManager::windowFocusChange(bool have_focus)
    {
    }

    void InputManager::windowVisibilityChange(bool visible)
    {
            //TODO: Pause game?
    }

    void InputManager::windowResized(int x, int y)
    {
        mOgre.windowResized(x,y);
    }

    void InputManager::windowClosed()
    {
        MWBase::Environment::get().getStateManager()->requestQuit();
    }

    void InputManager::toggleMainMenu()
    {
        if (MyGUI::InputManager::getInstance().isModalAny()) {
            MWBase::Environment::get().getWindowManager()->getCurrentModal()->exit();
            return;
        }

        if(!MWBase::Environment::get().getWindowManager()->isGuiMode()) //No open GUIs, open up the MainMenu
        {
            MWBase::Environment::get().getWindowManager()->pushGuiMode (MWGui::GM_MainMenu);
            MWBase::Environment::get().getSoundManager()->pauseSounds (MWBase::SoundManager::Play_TypeSfx);
        }
        else //Close current GUI
        {
            MWBase::Environment::get().getWindowManager()->exitCurrentGuiMode();
            MWBase::Environment::get().getSoundManager()->resumeSounds (MWBase::SoundManager::Play_TypeSfx);
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
        if (!mControlSwitch["playermagic"])
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
        if (!mControlSwitch["playerfighting"])
            return;

        MWMechanics::DrawState_ state = mPlayer->getDrawState();
        if (state == MWMechanics::DrawState_Spell || state == MWMechanics::DrawState_Nothing)
            mPlayer->setDrawState(MWMechanics::DrawState_Weapon);
        else
            mPlayer->setDrawState(MWMechanics::DrawState_Nothing);
    }

    void InputManager::rest()
    {
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
        mEngine.screenshot();

        std::vector<std::string> empty;
        MWBase::Environment::get().getWindowManager()->messageBox ("Screenshot saved", empty);
    }

    void InputManager::toggleInventory()
    {
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
        if (MyGUI::InputManager::getInstance ().isModalAny())
            return;

        if((!MWBase::Environment::get().getWindowManager()->isGuiMode()
            || MWBase::Environment::get().getWindowManager()->getMode() == MWGui::GM_Dialogue)
                && MWBase::Environment::get().getWindowManager ()->getJournalAllowed())
        {
            MWBase::Environment::get().getSoundManager()->playSound ("book open", 1.0, 1.0);
            MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Journal);
        }
        else if(MWBase::Environment::get().getWindowManager()->getMode() == MWGui::GM_Journal)
        {
            MWBase::Environment::get().getWindowManager()->exitCurrentGuiMode();
        }
    }

    void InputManager::quickKey (int index)
    {
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
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
            MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
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
                MWBase::Environment::get().getWindowManager()->getCurrentModal()->exit();
            }
            MWBase::Environment::get().getWindowManager()->exitCurrentGuiMode(); //And handle the actual main window
        }
    }

    void InputManager::activate()
    {
        if (mControlSwitch["playercontrols"])
            mEngine.activate();
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
        return mInputBinder->getChannel (id)->getValue () == 1;
    }

    void InputManager::loadKeyDefaults (bool force)
    {
        // using hardcoded key defaults is inevitable, if we want the configuration files to stay valid
        // across different versions of OpenMW (in the case where another input action is added)
        std::map<int, int> defaultKeyBindings;

        //Gets the Keyvalue from the Scancode; gives the button in the same place reguardless of keyboard format
        defaultKeyBindings[A_Activate] = SDL_GetKeyFromScancode(SDL_SCANCODE_SPACE);
        defaultKeyBindings[A_MoveBackward] = SDL_GetKeyFromScancode(SDL_SCANCODE_S);
        defaultKeyBindings[A_MoveForward] = SDL_GetKeyFromScancode(SDL_SCANCODE_W);
        defaultKeyBindings[A_MoveLeft] = SDL_GetKeyFromScancode(SDL_SCANCODE_A);
        defaultKeyBindings[A_MoveRight] = SDL_GetKeyFromScancode(SDL_SCANCODE_D);
        defaultKeyBindings[A_ToggleWeapon] = SDL_GetKeyFromScancode(SDL_SCANCODE_F);
        defaultKeyBindings[A_ToggleSpell] = SDL_GetKeyFromScancode(SDL_SCANCODE_R);
        defaultKeyBindings[A_QuickKeysMenu] = SDL_GetKeyFromScancode(SDL_SCANCODE_F1);
        defaultKeyBindings[A_Console] = SDL_GetKeyFromScancode(SDL_SCANCODE_F2);
        defaultKeyBindings[A_Run] = SDL_GetKeyFromScancode(SDL_SCANCODE_LSHIFT);
        defaultKeyBindings[A_Sneak] = SDL_GetKeyFromScancode(SDL_SCANCODE_LCTRL);
        defaultKeyBindings[A_AutoMove] = SDL_GetKeyFromScancode(SDL_SCANCODE_Q);
        defaultKeyBindings[A_Jump] = SDL_GetKeyFromScancode(SDL_SCANCODE_E);
        defaultKeyBindings[A_Journal] = SDL_GetKeyFromScancode(SDL_SCANCODE_J);
        defaultKeyBindings[A_Rest] = SDL_GetKeyFromScancode(SDL_SCANCODE_T);
        defaultKeyBindings[A_GameMenu] = SDL_GetKeyFromScancode(SDL_SCANCODE_ESCAPE);
        defaultKeyBindings[A_TogglePOV] = SDL_GetKeyFromScancode(SDL_SCANCODE_TAB);
        defaultKeyBindings[A_QuickKey1] = SDL_GetKeyFromScancode(SDL_SCANCODE_1);
        defaultKeyBindings[A_QuickKey2] = SDL_GetKeyFromScancode(SDL_SCANCODE_2);
        defaultKeyBindings[A_QuickKey3] = SDL_GetKeyFromScancode(SDL_SCANCODE_3);
        defaultKeyBindings[A_QuickKey4] = SDL_GetKeyFromScancode(SDL_SCANCODE_4);
        defaultKeyBindings[A_QuickKey5] = SDL_GetKeyFromScancode(SDL_SCANCODE_5);
        defaultKeyBindings[A_QuickKey6] = SDL_GetKeyFromScancode(SDL_SCANCODE_6);
        defaultKeyBindings[A_QuickKey7] = SDL_GetKeyFromScancode(SDL_SCANCODE_7);
        defaultKeyBindings[A_QuickKey8] = SDL_GetKeyFromScancode(SDL_SCANCODE_8);
        defaultKeyBindings[A_QuickKey9] = SDL_GetKeyFromScancode(SDL_SCANCODE_9);
        defaultKeyBindings[A_QuickKey10] = SDL_GetKeyFromScancode(SDL_SCANCODE_0);
        defaultKeyBindings[A_Screenshot] = SDL_GetKeyFromScancode(SDL_SCANCODE_F12);
        defaultKeyBindings[A_ToggleHUD] = SDL_GetKeyFromScancode(SDL_SCANCODE_F11);
        defaultKeyBindings[A_AlwaysRun] = SDL_GetKeyFromScancode(SDL_SCANCODE_Y);
        defaultKeyBindings[A_QuickSave] = SDL_GetKeyFromScancode(SDL_SCANCODE_F5);
        defaultKeyBindings[A_QuickLoad] = SDL_GetKeyFromScancode(SDL_SCANCODE_F9);

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
