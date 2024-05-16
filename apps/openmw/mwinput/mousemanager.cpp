#include "mousemanager.hpp"

#include <MyGUI_Button.h>
#include <MyGUI_InputManager.h>
#include <MyGUI_RenderManager.h>

#include <components/sdlutil/sdlinputwrapper.hpp>
#include <components/sdlutil/sdlmappings.hpp>
#include <components/settings/values.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/inputmanager.hpp"
#include "../mwbase/luamanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwgui/settingswindow.hpp"

#include "../mwworld/player.hpp"

#include "actions.hpp"
#include "bindingsmanager.hpp"

namespace MWInput
{
    MouseManager::MouseManager(
        BindingsManager* bindingsManager, SDLUtil::InputWrapper* inputWrapper, SDL_Window* window)
        : mBindingsManager(bindingsManager)
        , mInputWrapper(inputWrapper)
        , mGuiCursorX(0)
        , mGuiCursorY(0)
        , mMouseWheel(0)
        , mMouseLookEnabled(false)
        , mGuiCursorEnabled(true)
        , mMouseMoveX(0)
        , mMouseMoveY(0)
    {
        int w, h;
        SDL_GetWindowSize(window, &w, &h);

        float uiScale = MWBase::Environment::get().getWindowManager()->getScalingFactor();
        mGuiCursorX = w / (2.f * uiScale);
        mGuiCursorY = h / (2.f * uiScale);
    }

    void MouseManager::mouseMoved(const SDLUtil::MouseMotionEvent& arg)
    {
        mBindingsManager->mouseMoved(arg);

        MWBase::InputManager* input = MWBase::Environment::get().getInputManager();
        input->setJoystickLastUsed(false);
        input->resetIdleTime();

        if (mGuiCursorEnabled)
        {
            input->setGamepadGuiCursorEnabled(true);

            // We keep track of our own mouse position, so that moving the mouse while in
            // game mode does not move the position of the GUI cursor
            float uiScale = MWBase::Environment::get().getWindowManager()->getScalingFactor();
            mGuiCursorX = static_cast<float>(arg.x) / uiScale;
            mGuiCursorY = static_cast<float>(arg.y) / uiScale;

            mMouseWheel = static_cast<int>(arg.z);

            MyGUI::InputManager::getInstance().injectMouseMove(
                static_cast<int>(mGuiCursorX), static_cast<int>(mGuiCursorY), mMouseWheel);
            // FIXME: inject twice to force updating focused widget states (tooltips) resulting from changing the
            // viewport by scroll wheel
            MyGUI::InputManager::getInstance().injectMouseMove(
                static_cast<int>(mGuiCursorX), static_cast<int>(mGuiCursorY), mMouseWheel);

            MWBase::Environment::get().getWindowManager()->setCursorActive(true);
        }

        if (mMouseLookEnabled && !input->controlsDisabled())
        {
            MWBase::World* world = MWBase::Environment::get().getWorld();

            const float cameraSensitivity = Settings::input().mCameraSensitivity;
            float x = arg.xrel * cameraSensitivity * (Settings::input().mInvertXAxis ? -1 : 1) / 256.f;
            float y = arg.yrel * cameraSensitivity * (Settings::input().mInvertYAxis ? -1 : 1)
                * Settings::input().mCameraYMultiplier / 256.f;

            float rot[3];
            rot[0] = -y;
            rot[1] = 0.0f;
            rot[2] = -x;

            // Only actually turn player when we're not in vanity mode
            if (!world->vanityRotateCamera(rot) && input->getControlSwitch("playerlooking"))
            {
                MWWorld::Player& player = world->getPlayer();
                player.yaw(x);
                player.pitch(y);
            }
            else if (!input->getControlSwitch("playerlooking"))
                MWBase::Environment::get().getWorld()->disableDeferredPreviewRotation();
        }
    }

    void MouseManager::mouseReleased(const SDL_MouseButtonEvent& arg, Uint8 id)
    {
        MWBase::Environment::get().getInputManager()->setJoystickLastUsed(false);

        if (mBindingsManager->isDetectingBindingState())
        {
            mBindingsManager->mouseReleased(arg, id);
        }
        else
        {
            bool guiMode = MWBase::Environment::get().getWindowManager()->isGuiMode();
            guiMode = MyGUI::InputManager::getInstance().injectMouseRelease(static_cast<int>(mGuiCursorX),
                          static_cast<int>(mGuiCursorY), SDLUtil::sdlMouseButtonToMyGui(id))
                && guiMode;

            if (mBindingsManager->isDetectingBindingState())
                return; // don't allow same mouseup to bind as initiated bind

            mBindingsManager->setPlayerControlsEnabled(!guiMode);
            mBindingsManager->mouseReleased(arg, id);
        }

        MWBase::Environment::get().getLuaManager()->inputEvent(
            { MWBase::LuaManager::InputEvent::MouseButtonReleased, arg.button });
    }

    void MouseManager::mouseWheelMoved(const SDL_MouseWheelEvent& arg)
    {
        MWBase::InputManager* input = MWBase::Environment::get().getInputManager();
        if (mBindingsManager->isDetectingBindingState() || !input->controlsDisabled())
        {
            mBindingsManager->mouseWheelMoved(arg);
        }

        input->setJoystickLastUsed(false);
        MWBase::Environment::get().getLuaManager()->inputEvent({ MWBase::LuaManager::InputEvent::MouseWheel,
            MWBase::LuaManager::InputEvent::WheelChange{ arg.x, arg.y } });
    }

    void MouseManager::mousePressed(const SDL_MouseButtonEvent& arg, Uint8 id)
    {
        MWBase::InputManager* input = MWBase::Environment::get().getInputManager();
        input->setJoystickLastUsed(false);
        bool guiMode = false;

        if (id == SDL_BUTTON_LEFT || id == SDL_BUTTON_RIGHT) // MyGUI only uses these mouse events
        {
            guiMode = MWBase::Environment::get().getWindowManager()->isGuiMode();
            guiMode = MyGUI::InputManager::getInstance().injectMousePress(static_cast<int>(mGuiCursorX),
                          static_cast<int>(mGuiCursorY), SDLUtil::sdlMouseButtonToMyGui(id))
                && guiMode;
            if (MyGUI::InputManager::getInstance().getMouseFocusWidget() != nullptr)
            {
                MyGUI::Button* b
                    = MyGUI::InputManager::getInstance().getMouseFocusWidget()->castType<MyGUI::Button>(false);
                if (b && b->getEnabled() && id == SDL_BUTTON_LEFT)
                {
                    MWBase::Environment::get().getWindowManager()->playSound(ESM::RefId::stringRefId("Menu Click"));
                }
            }
            MWBase::Environment::get().getWindowManager()->setCursorActive(true);
        }

        mBindingsManager->setPlayerControlsEnabled(!guiMode);

        // Don't trigger any mouse bindings while in settings menu, otherwise rebinding controls becomes impossible
        // Also do not trigger bindings when input controls are disabled, e.g. during save loading
        const MWGui::SettingsWindow* settingsWindow
            = MWBase::Environment::get().getWindowManager()->getSettingsWindow();
        if ((!settingsWindow || !settingsWindow->isVisible()) && !input->controlsDisabled())
        {
            mBindingsManager->mousePressed(arg, id);
        }
        MWBase::Environment::get().getLuaManager()->inputEvent(
            { MWBase::LuaManager::InputEvent::MouseButtonPressed, arg.button });
    }

    void MouseManager::updateCursorMode()
    {
        bool grab = !MWBase::Environment::get().getWindowManager()->containsMode(MWGui::GM_MainMenu)
            && !MWBase::Environment::get().getWindowManager()->isConsoleMode();

        bool wasRelative = mInputWrapper->getMouseRelative();
        bool isRelative = !MWBase::Environment::get().getWindowManager()->isGuiMode();

        // don't keep the pointer away from the window edge in gui mode
        // stop using raw mouse motions and switch to system cursor movements
        mInputWrapper->setMouseRelative(isRelative);

        // we let the mouse escape in the main menu
        mInputWrapper->setGrabPointer(grab && (Settings::input().mGrabCursor || isRelative));

        // we switched to non-relative mode, move our cursor to where the in-game
        // cursor is
        if (!isRelative && wasRelative != isRelative)
        {
            warpMouse();
        }
    }

    void MouseManager::update(float dt)
    {
        SDL_GetRelativeMouseState(&mMouseMoveX, &mMouseMoveY);

        if (!mMouseLookEnabled)
            return;

        float xAxis = mBindingsManager->getActionValue(A_LookLeftRight) * 2.0f - 1.0f;
        float yAxis = mBindingsManager->getActionValue(A_LookUpDown) * 2.0f - 1.0f;
        if (xAxis == 0 && yAxis == 0)
            return;

        const float cameraSensitivity = Settings::input().mCameraSensitivity;
        const float rot[3] = {
            -yAxis * dt * 1000.0f * cameraSensitivity * (Settings::input().mInvertYAxis ? -1 : 1)
                * Settings::input().mCameraYMultiplier / 256.f,
            0.0f,
            -xAxis * dt * 1000.0f * cameraSensitivity * (Settings::input().mInvertXAxis ? -1 : 1) / 256.f,
        };

        // Only actually turn player when we're not in vanity mode
        bool playerLooking = MWBase::Environment::get().getInputManager()->getControlSwitch("playerlooking");
        if (!MWBase::Environment::get().getWorld()->vanityRotateCamera(rot) && playerLooking)
        {
            MWWorld::Player& player = MWBase::Environment::get().getWorld()->getPlayer();
            player.yaw(-rot[2]);
            player.pitch(-rot[0]);
        }
        else if (!playerLooking)
            MWBase::Environment::get().getWorld()->disableDeferredPreviewRotation();

        MWBase::Environment::get().getInputManager()->resetIdleTime();
    }

    bool MouseManager::injectMouseButtonPress(Uint8 button)
    {
        return MyGUI::InputManager::getInstance().injectMousePress(
            static_cast<int>(mGuiCursorX), static_cast<int>(mGuiCursorY), SDLUtil::sdlMouseButtonToMyGui(button));
    }

    bool MouseManager::injectMouseButtonRelease(Uint8 button)
    {
        return MyGUI::InputManager::getInstance().injectMouseRelease(
            static_cast<int>(mGuiCursorX), static_cast<int>(mGuiCursorY), SDLUtil::sdlMouseButtonToMyGui(button));
    }

    void MouseManager::injectMouseMove(float xMove, float yMove, float mouseWheelMove)
    {
        mGuiCursorX += xMove;
        mGuiCursorY += yMove;
        mMouseWheel += mouseWheelMove;

        const MyGUI::IntSize& viewSize = MyGUI::RenderManager::getInstance().getViewSize();
        mGuiCursorX = std::clamp<float>(mGuiCursorX, 0.f, viewSize.width - 1);
        mGuiCursorY = std::clamp<float>(mGuiCursorY, 0.f, viewSize.height - 1);

        MyGUI::InputManager::getInstance().injectMouseMove(
            static_cast<int>(mGuiCursorX), static_cast<int>(mGuiCursorY), static_cast<int>(mMouseWheel));
    }

    void MouseManager::warpMouse()
    {
        float guiUiScale = Settings::gui().mScalingFactor;
        mInputWrapper->warpMouse(
            static_cast<int>(mGuiCursorX * guiUiScale), static_cast<int>(mGuiCursorY * guiUiScale));
    }
}
