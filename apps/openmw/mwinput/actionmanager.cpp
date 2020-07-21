#include "actionmanager.hpp"

#include <MyGUI_InputManager.h>

#include <SDL_keyboard.h>

#include <components/settings/settings.hpp>

#include "../mwbase/inputmanager.hpp"
#include "../mwbase/statemanager.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/player.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/actorutil.hpp"

#include "actions.hpp"
#include "bindingsmanager.hpp"

namespace MWInput
{
    const float ZOOM_SCALE = 120.f; /// Used for scrolling camera in and out

    ActionManager::ActionManager(BindingsManager* bindingsManager,
            osgViewer::ScreenCaptureHandler::CaptureOperation* screenCaptureOperation,
            osg::ref_ptr<osgViewer::Viewer> viewer,
            osg::ref_ptr<osgViewer::ScreenCaptureHandler> screenCaptureHandler)
        : mBindingsManager(bindingsManager)
        , mViewer(viewer)
        , mScreenCaptureHandler(screenCaptureHandler)
        , mScreenCaptureOperation(screenCaptureOperation)
        , mAlwaysRunActive(Settings::Manager::getBool("always run", "Input"))
        , mSneaking(false)
        , mAttemptJump(false)
        , mOverencumberedMessageDelay(0.f)
        , mPreviewPOVDelay(0.f)
        , mTimeIdle(0.f)
    {
    }

    void ActionManager::update(float dt, bool triedToMove)
    {
        // Disable movement in Gui mode
        if (MWBase::Environment::get().getWindowManager()->isGuiMode()
            || MWBase::Environment::get().getStateManager()->getState() != MWBase::StateManager::State_Running)
        {
            mAttemptJump = false;
            return;
        }

        // Configure player movement according to keyboard input. Actual movement will
        // be done in the physics system.
        if (MWBase::Environment::get().getInputManager()->getControlSwitch("playercontrols"))
        {
            bool alwaysRunAllowed = false;

            MWWorld::Player& player = MWBase::Environment::get().getWorld()->getPlayer();

            if (mBindingsManager->actionIsActive(A_MoveLeft) != mBindingsManager->actionIsActive(A_MoveRight))
            {
                alwaysRunAllowed = true;
                triedToMove = true;
                player.setLeftRight(mBindingsManager->actionIsActive(A_MoveRight) ? 1 : -1);
            }

            if (mBindingsManager->actionIsActive(A_MoveForward) != mBindingsManager->actionIsActive(A_MoveBackward))
            {
                alwaysRunAllowed = true;
                triedToMove = true;
                player.setAutoMove (false);
                player.setForwardBackward(mBindingsManager->actionIsActive(A_MoveForward) ? 1 : -1);
            }

            if (player.getAutoMove())
            {
                alwaysRunAllowed = true;
                triedToMove = true;
                player.setForwardBackward (1);
            }

            if (mAttemptJump && MWBase::Environment::get().getInputManager()->getControlSwitch("playerjumping"))
            {
                player.setUpDown(1);
                triedToMove = true;
                mOverencumberedMessageDelay = 0.f;
            }

            // if player tried to start moving, but can't (due to being overencumbered), display a notification.
            if (triedToMove)
            {
                MWWorld::Ptr playerPtr = MWBase::Environment::get().getWorld ()->getPlayerPtr();
                mOverencumberedMessageDelay -= dt;
                if (playerPtr.getClass().getEncumbrance(playerPtr) > playerPtr.getClass().getCapacity(playerPtr))
                {
                    player.setAutoMove (false);
                    if (mOverencumberedMessageDelay <= 0)
                    {
                        MWBase::Environment::get().getWindowManager()->messageBox("#{sNotifyMessage59}");
                        mOverencumberedMessageDelay = 1.0;
                    }
                }
            }

            if (MWBase::Environment::get().getInputManager()->getControlSwitch("playerviewswitch"))
            {
                const float switchLimit = 0.25;
                MWBase::World* world = MWBase::Environment::get().getWorld();
                if (mBindingsManager->actionIsActive(A_TogglePOV))
                {
                    if (world->isFirstPerson() ? mPreviewPOVDelay > switchLimit : mPreviewPOVDelay == 0)
                        world->togglePreviewMode(true);
                    mPreviewPOVDelay += dt;
                }
                else
                {
                    //disable preview mode
                    if (mPreviewPOVDelay > 0)
                        world->togglePreviewMode(false);
                    if (mPreviewPOVDelay > 0.f && mPreviewPOVDelay <= switchLimit)
                        world->togglePOV();
                    mPreviewPOVDelay = 0.f;
                }
            }

            if (triedToMove)
                MWBase::Environment::get().getInputManager()->resetIdleTime();

            static const bool isToggleSneak = Settings::Manager::getBool("toggle sneak", "Input");
            if (!isToggleSneak)
            {
                if(!MWBase::Environment::get().getInputManager()->joystickLastUsed())
                    player.setSneak(mBindingsManager->actionIsActive(A_Sneak));
            }

            float xAxis = mBindingsManager->getActionValue(A_MoveLeftRight);
            float yAxis = mBindingsManager->getActionValue(A_MoveForwardBackward);
            bool isRunning = xAxis > .75 || xAxis < .25 || yAxis > .75 || yAxis < .25;
            if ((mAlwaysRunActive && alwaysRunAllowed) || isRunning)
                player.setRunState(!mBindingsManager->actionIsActive(A_Run));
            else
                player.setRunState(mBindingsManager->actionIsActive(A_Run));
        }

        if (mBindingsManager->actionIsActive(A_MoveForward) ||
            mBindingsManager->actionIsActive(A_MoveBackward) ||
            mBindingsManager->actionIsActive(A_MoveLeft) ||
            mBindingsManager->actionIsActive(A_MoveRight) ||
            mBindingsManager->actionIsActive(A_Jump) ||
            mBindingsManager->actionIsActive(A_Sneak) ||
            mBindingsManager->actionIsActive(A_TogglePOV) ||
            mBindingsManager->actionIsActive(A_ZoomIn) ||
            mBindingsManager->actionIsActive(A_ZoomOut))
        {
            resetIdleTime();
        }
        else
        {
            updateIdleTime(dt);
        }

        mAttemptJump = false;
    }

    void ActionManager::resetIdleTime()
    {
        if (mTimeIdle < 0)
            MWBase::Environment::get().getWorld()->toggleVanityMode(false);
        mTimeIdle = 0.f;
    }

    void ActionManager::updateIdleTime(float dt)
    {
        static const float vanityDelay = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>()
                .find("fVanityDelay")->mValue.getFloat();
        if (mTimeIdle >= 0.f)
            mTimeIdle += dt;
        if (mTimeIdle > vanityDelay)
        {
            MWBase::Environment::get().getWorld()->toggleVanityMode(true);
            mTimeIdle = -1.f;
        }
    }

    void ActionManager::executeAction(int action)
    {
        // trigger action activated
        switch (action)
        {
        case A_GameMenu:
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
            MWBase::Environment::get().getInputManager()->resetIdleTime();
            activate();
            break;
        case A_MoveLeft:
        case A_MoveRight:
        case A_MoveForward:
        case A_MoveBackward:
            handleGuiArrowKey(action);
            break;
        case A_Journal:
            toggleJournal();
            break;
        case A_AutoMove:
            toggleAutoMove();
            break;
        case A_AlwaysRun:
            toggleWalking();
            break;
        case A_ToggleWeapon:
            toggleWeapon();
            break;
        case A_Rest:
            rest();
            break;
        case A_ToggleSpell:
            toggleSpell();
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
            MWBase::Environment::get().getWindowManager()->toggleHud();
            break;
        case A_ToggleDebug:
            MWBase::Environment::get().getWindowManager()->toggleDebugWindow();
            break;
        case A_ZoomIn:
            if (MWBase::Environment::get().getInputManager()->getControlSwitch("playerviewswitch") && MWBase::Environment::get().getInputManager()->getControlSwitch("playercontrols") && !MWBase::Environment::get().getWindowManager()->isGuiMode())
                MWBase::Environment::get().getWorld()->setCameraDistance(ZOOM_SCALE, true, true);
            break;
        case A_ZoomOut:
            if (MWBase::Environment::get().getInputManager()->getControlSwitch("playerviewswitch") && MWBase::Environment::get().getInputManager()->getControlSwitch("playercontrols") && !MWBase::Environment::get().getWindowManager()->isGuiMode())
                MWBase::Environment::get().getWorld()->setCameraDistance(-ZOOM_SCALE, true, true);
            break;
        case A_QuickSave:
            quickSave();
            break;
        case A_QuickLoad:
            quickLoad();
            break;
        case A_CycleSpellLeft:
            if (checkAllowedToUseItems() && MWBase::Environment::get().getWindowManager()->isAllowed(MWGui::GW_Magic))
                MWBase::Environment::get().getWindowManager()->cycleSpell(false);
            break;
        case A_CycleSpellRight:
            if (checkAllowedToUseItems() && MWBase::Environment::get().getWindowManager()->isAllowed(MWGui::GW_Magic))
                MWBase::Environment::get().getWindowManager()->cycleSpell(true);
            break;
        case A_CycleWeaponLeft:
            if (checkAllowedToUseItems() && MWBase::Environment::get().getWindowManager()->isAllowed(MWGui::GW_Inventory))
                MWBase::Environment::get().getWindowManager()->cycleWeapon(false);
            break;
        case A_CycleWeaponRight:
            if (checkAllowedToUseItems() && MWBase::Environment::get().getWindowManager()->isAllowed(MWGui::GW_Inventory))
                MWBase::Environment::get().getWindowManager()->cycleWeapon(true);
            break;
        case A_Sneak:
            static const bool isToggleSneak = Settings::Manager::getBool("toggle sneak", "Input");
            if (isToggleSneak)
            {
                toggleSneaking();
            }
            break;
        }
    }

    bool ActionManager::checkAllowedToUseItems() const
    {
        MWWorld::Ptr player = MWMechanics::getPlayer();
        if (player.getClass().getNpcStats(player).isWerewolf())
        {
            // Cannot use items or spells while in werewolf form
            MWBase::Environment::get().getWindowManager()->messageBox("#{sWerewolfRefusal}");
            return false;
        }
        return true;
    }

    void ActionManager::screenshot()
    {
        bool regularScreenshot = true;

        std::string settingStr;

        settingStr = Settings::Manager::getString("screenshot type","Video");
        regularScreenshot = settingStr.size() == 0 || settingStr.compare("regular") == 0;

        if (regularScreenshot)
        {
            mScreenCaptureHandler->setFramesToCapture(1);
            mScreenCaptureHandler->captureNextFrame(*mViewer);
        }
        else
        {
            osg::ref_ptr<osg::Image> screenshot (new osg::Image);

            if (MWBase::Environment::get().getWorld()->screenshot360(screenshot.get(), settingStr))
            {
                (*mScreenCaptureOperation) (*(screenshot.get()), 0);
                // FIXME: mScreenCaptureHandler->getCaptureOperation() causes crash for some reason
            }
        }
    }

    void ActionManager::toggleMainMenu()
    {
        if (MyGUI::InputManager::getInstance().isModalAny())
        {
            MWBase::Environment::get().getWindowManager()->exitCurrentModal();
            return;
        }

        if (MWBase::Environment::get().getWindowManager()->isConsoleMode())
        {
            MWBase::Environment::get().getWindowManager()->toggleConsole();
            return;
        }

        if (!MWBase::Environment::get().getWindowManager()->isGuiMode()) //No open GUIs, open up the MainMenu
        {
            MWBase::Environment::get().getWindowManager()->pushGuiMode (MWGui::GM_MainMenu);
        }
        else //Close current GUI
        {
            MWBase::Environment::get().getWindowManager()->exitCurrentGuiMode();
        }
    }

    void ActionManager::toggleSpell()
    {
        if (MWBase::Environment::get().getWindowManager()->isGuiMode()) return;

        // Not allowed before the magic window is accessible
        if (!MWBase::Environment::get().getInputManager()->getControlSwitch("playermagic") || !MWBase::Environment::get().getInputManager()->getControlSwitch("playercontrols"))
            return;

        if (!checkAllowedToUseItems())
            return;

        // Not allowed if no spell selected
        MWWorld::Player& player = MWBase::Environment::get().getWorld()->getPlayer();
        MWWorld::InventoryStore& inventory = player.getPlayer().getClass().getInventoryStore(player.getPlayer());
        if (MWBase::Environment::get().getWindowManager()->getSelectedSpell().empty() &&
            inventory.getSelectedEnchantItem() == inventory.end())
            return;

        if (MWBase::Environment::get().getMechanicsManager()->isAttackingOrSpell(player.getPlayer()))
            return;

        MWMechanics::DrawState_ state = player.getDrawState();
        if (state == MWMechanics::DrawState_Weapon || state == MWMechanics::DrawState_Nothing)
            player.setDrawState(MWMechanics::DrawState_Spell);
        else
            player.setDrawState(MWMechanics::DrawState_Nothing);
    }

    void ActionManager::quickLoad()
    {
        if (!MyGUI::InputManager::getInstance().isModalAny())
            MWBase::Environment::get().getStateManager()->quickLoad();
    }

    void ActionManager::quickSave()
    {
        if (!MyGUI::InputManager::getInstance().isModalAny())
            MWBase::Environment::get().getStateManager()->quickSave();
    }

    void ActionManager::toggleWeapon()
    {
        if (MWBase::Environment::get().getWindowManager()->isGuiMode()) return;

        // Not allowed before the inventory window is accessible
        if (!MWBase::Environment::get().getInputManager()->getControlSwitch("playerfighting") || !MWBase::Environment::get().getInputManager()->getControlSwitch("playercontrols"))
            return;

        MWWorld::Player& player = MWBase::Environment::get().getWorld()->getPlayer();
        // We want to interrupt animation only if attack is preparing, but still is not triggered
        // Otherwise we will get a "speedshooting" exploit, when player can skip reload animation by hitting "Toggle Weapon" key twice
        if (MWBase::Environment::get().getMechanicsManager()->isAttackPreparing(player.getPlayer()))
            player.setAttackingOrSpell(false);
        else if (MWBase::Environment::get().getMechanicsManager()->isAttackingOrSpell(player.getPlayer()))
            return;

        MWMechanics::DrawState_ state = player.getDrawState();
        if (state == MWMechanics::DrawState_Spell || state == MWMechanics::DrawState_Nothing)
            player.setDrawState(MWMechanics::DrawState_Weapon);
        else
            player.setDrawState(MWMechanics::DrawState_Nothing);
    }

    void ActionManager::rest()
    {
        if (!MWBase::Environment::get().getInputManager()->getControlSwitch("playercontrols"))
            return;

        if (!MWBase::Environment::get().getWindowManager()->getRestEnabled() || MWBase::Environment::get().getWindowManager()->isGuiMode())
            return;

        MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Rest); //Open rest GUI
    }

    void ActionManager::toggleInventory()
    {
        if (!MWBase::Environment::get().getInputManager()->getControlSwitch("playercontrols"))
            return;

        if (MyGUI::InputManager::getInstance().isModalAny())
            return;

        if (MWBase::Environment::get().getWindowManager()->isConsoleMode())
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

    void ActionManager::toggleConsole()
    {
        if (MyGUI::InputManager::getInstance().isModalAny())
            return;

        MWBase::Environment::get().getWindowManager()->toggleConsole();
    }

    void ActionManager::toggleJournal()
    {
        if (!MWBase::Environment::get().getInputManager()->getControlSwitch("playercontrols"))
            return;
        if (MyGUI::InputManager::getInstance ().isModalAny())
            return;

        if (MWBase::Environment::get().getWindowManager()->getMode() != MWGui::GM_Journal
                && MWBase::Environment::get().getWindowManager()->getMode() != MWGui::GM_MainMenu
                && MWBase::Environment::get().getWindowManager()->getMode() != MWGui::GM_Settings
                && MWBase::Environment::get().getWindowManager ()->getJournalAllowed())
        {
            MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Journal);
        }
        else if (MWBase::Environment::get().getWindowManager()->containsMode(MWGui::GM_Journal))
        {
            MWBase::Environment::get().getWindowManager()->removeGuiMode(MWGui::GM_Journal);
        }
    }

    void ActionManager::quickKey (int index)
    {
        if (!MWBase::Environment::get().getInputManager()->getControlSwitch("playercontrols") || !MWBase::Environment::get().getInputManager()->getControlSwitch("playerfighting") || !MWBase::Environment::get().getInputManager()->getControlSwitch("playermagic"))
            return;
        if (!checkAllowedToUseItems())
            return;

        if (MWBase::Environment::get().getWorld()->getGlobalFloat ("chargenstate")!=-1)
            return;

        if (!MWBase::Environment::get().getWindowManager()->isGuiMode())
            MWBase::Environment::get().getWindowManager()->activateQuickKey (index);
    }

    void ActionManager::showQuickKeysMenu()
    {
        if (!MWBase::Environment::get().getWindowManager()->isGuiMode ()
                && MWBase::Environment::get().getWorld()->getGlobalFloat ("chargenstate")==-1)
        {
            if (!checkAllowedToUseItems())
                return;

            MWBase::Environment::get().getWindowManager()->pushGuiMode (MWGui::GM_QuickKeysMenu);
        }
        else if (MWBase::Environment::get().getWindowManager()->getMode () == MWGui::GM_QuickKeysMenu)
        {
            while (MyGUI::InputManager::getInstance().isModalAny())
            { //Handle any open Modal windows
                MWBase::Environment::get().getWindowManager()->exitCurrentModal();
            }
            MWBase::Environment::get().getWindowManager()->exitCurrentGuiMode(); //And handle the actual main window
        }
    }

    void ActionManager::activate()
    {
        if (MWBase::Environment::get().getWindowManager()->isGuiMode())
        {
            bool joystickUsed = MWBase::Environment::get().getInputManager()->joystickLastUsed();
            if (!SDL_IsTextInputActive() && !mBindingsManager->isLeftOrRightButton(A_Activate, joystickUsed))
                MWBase::Environment::get().getWindowManager()->injectKeyPress(MyGUI::KeyCode::Return, 0, false);
        }
        else if (MWBase::Environment::get().getInputManager()->getControlSwitch("playercontrols"))
        {
            MWWorld::Player& player = MWBase::Environment::get().getWorld()->getPlayer();
            player.activate();
        }
    }

    void ActionManager::toggleAutoMove()
    {
        if (MWBase::Environment::get().getWindowManager()->isGuiMode()) return;

        if (MWBase::Environment::get().getInputManager()->getControlSwitch("playercontrols"))
        {
            MWWorld::Player& player = MWBase::Environment::get().getWorld()->getPlayer();
            player.setAutoMove (!player.getAutoMove());
        }
    }

    void ActionManager::toggleWalking()
    {
        if (MWBase::Environment::get().getWindowManager()->isGuiMode() || SDL_IsTextInputActive()) return;
        mAlwaysRunActive = !mAlwaysRunActive;

        Settings::Manager::setBool("always run", "Input", mAlwaysRunActive);
    }

    void ActionManager::toggleSneaking()
    {
        if (MWBase::Environment::get().getWindowManager()->isGuiMode()) return;
        if (!MWBase::Environment::get().getInputManager()->getControlSwitch("playercontrols")) return;
        mSneaking = !mSneaking;
        MWWorld::Player& player = MWBase::Environment::get().getWorld()->getPlayer();
        player.setSneak(mSneaking);
    }

    void ActionManager::handleGuiArrowKey(int action)
    {
        bool joystickUsed = MWBase::Environment::get().getInputManager()->joystickLastUsed();
        // This is currently keyboard-specific code
        // TODO: see if GUI controls can be refactored into a single function
        if (joystickUsed)
            return;

        if (SDL_IsTextInputActive())
            return;

        if (mBindingsManager->isLeftOrRightButton(action, joystickUsed))
            return;

        MyGUI::KeyCode key;
        switch (action)
        {
        case A_MoveLeft:
            key = MyGUI::KeyCode::ArrowLeft;
            break;
        case A_MoveRight:
            key = MyGUI::KeyCode::ArrowRight;
            break;
        case A_MoveForward:
            key = MyGUI::KeyCode::ArrowUp;
            break;
        case A_MoveBackward:
        default:
            key = MyGUI::KeyCode::ArrowDown;
            break;
        }

        MWBase::Environment::get().getWindowManager()->injectKeyPress(key, 0, false);
    }
}
