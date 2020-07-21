#include "mousemanager.hpp"

#include <MyGUI_Button.h>
#include <MyGUI_InputManager.h>
#include <MyGUI_RenderManager.h>
#include <MyGUI_Widget.h>

#include <components/debug/debuglog.hpp>
#include <components/sdlutil/sdlinputwrapper.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/inputmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/player.hpp"

#include "actions.hpp"
#include "bindingsmanager.hpp"
#include "sdlmappings.hpp"

namespace MWInput
{
    MouseManager::MouseManager(BindingsManager* bindingsManager, SDLUtil::InputWrapper* inputWrapper, SDL_Window* window)
        : mInvertX(Settings::Manager::getBool("invert x axis", "Input"))
        , mInvertY(Settings::Manager::getBool("invert y axis", "Input"))
        , mGrabCursor(Settings::Manager::getBool("grab cursor", "Input"))
        , mCameraSensitivity(Settings::Manager::getFloat("camera sensitivity", "Input"))
        , mCameraYMultiplier(Settings::Manager::getFloat("camera y multiplier", "Input"))
        , mBindingsManager(bindingsManager)
        , mInputWrapper(inputWrapper)
        , mInvUiScalingFactor(1.f)
        , mGuiCursorX(0)
        , mGuiCursorY(0)
        , mMouseWheel(0)
        , mMouseLookEnabled(false)
        , mGuiCursorEnabled(true)
    {
        float uiScale = Settings::Manager::getFloat("scaling factor", "GUI");
        if (uiScale != 0.f)
            mInvUiScalingFactor = 1.f / uiScale;

        int w,h;
        SDL_GetWindowSize(window, &w, &h);

        mGuiCursorX = mInvUiScalingFactor * w / 2.f;
        mGuiCursorY = mInvUiScalingFactor * h / 2.f;
    }

    void MouseManager::processChangedSettings(const Settings::CategorySettingVector& changed)
    {
        for (const auto& setting : changed)
        {
            if (setting.first == "Input" && setting.second == "invert x axis")
                mInvertX = Settings::Manager::getBool("invert x axis", "Input");

            if (setting.first == "Input" && setting.second == "invert y axis")
                mInvertY = Settings::Manager::getBool("invert y axis", "Input");

            if (setting.first == "Input" && setting.second == "camera sensitivity")
                mCameraSensitivity = Settings::Manager::getFloat("camera sensitivity", "Input");

            if (setting.first == "Input" && setting.second == "grab cursor")
                mGrabCursor = Settings::Manager::getBool("grab cursor", "Input");
        }
    }

    void MouseManager::mouseMoved(const SDLUtil::MouseMotionEvent &arg)
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
            mGuiCursorX = static_cast<float>(arg.x) * mInvUiScalingFactor;
            mGuiCursorY = static_cast<float>(arg.y) * mInvUiScalingFactor;

            mMouseWheel = static_cast<int>(arg.z);

            MyGUI::InputManager::getInstance().injectMouseMove(static_cast<int>(mGuiCursorX), static_cast<int>(mGuiCursorY), mMouseWheel);
            // FIXME: inject twice to force updating focused widget states (tooltips) resulting from changing the viewport by scroll wheel
            MyGUI::InputManager::getInstance().injectMouseMove(static_cast<int>(mGuiCursorX), static_cast<int>(mGuiCursorY), mMouseWheel);

            MWBase::Environment::get().getWindowManager()->setCursorActive(true);
        }

        if (mMouseLookEnabled && !input->controlsDisabled())
        {
            float x = arg.xrel * mCameraSensitivity * (mInvertX ? -1 : 1) / 256.f;
            float y = arg.yrel * mCameraSensitivity * (mInvertY ? -1 : 1) * mCameraYMultiplier / 256.f;

            float rot[3];
            rot[0] = -y;
            rot[1] = 0.0f;
            rot[2] = -x;

            // Only actually turn player when we're not in vanity mode
            if (!MWBase::Environment::get().getWorld()->vanityRotateCamera(rot) && input->getControlSwitch("playerlooking"))
            {
                MWWorld::Player& player = MWBase::Environment::get().getWorld()->getPlayer();
                player.yaw(x);
                player.pitch(y);
            }
            else if (!input->getControlSwitch("playerlooking"))
                MWBase::Environment::get().getWorld()->disableDeferredPreviewRotation();

            if (arg.zrel && input->getControlSwitch("playerviewswitch") && input->getControlSwitch("playercontrols")) //Check to make sure you are allowed to zoomout and there is a change
            {
                MWBase::Environment::get().getWorld()->changeVanityModeScale(static_cast<float>(arg.zrel));
            }
        }
    }

    void MouseManager::mouseReleased(const SDL_MouseButtonEvent &arg, Uint8 id)
    {
        MWBase::Environment::get().getInputManager()->setJoystickLastUsed(false);

        if (mBindingsManager->isDetectingBindingState())
        {
            mBindingsManager->mouseReleased(arg, id);
        }
        else
        {
            bool guiMode = MWBase::Environment::get().getWindowManager()->isGuiMode();
            guiMode = MyGUI::InputManager::getInstance().injectMouseRelease(static_cast<int>(mGuiCursorX), static_cast<int>(mGuiCursorY), sdlButtonToMyGUI(id)) && guiMode;

            if (mBindingsManager->isDetectingBindingState())
                return; // don't allow same mouseup to bind as initiated bind

            mBindingsManager->setPlayerControlsEnabled(!guiMode);
            mBindingsManager->mouseReleased(arg, id);
        }
    }

    void MouseManager::mouseWheelMoved(const SDL_MouseWheelEvent &arg)
    {
        MWBase::InputManager* input = MWBase::Environment::get().getInputManager();
        if (mBindingsManager->isDetectingBindingState() || !input->controlsDisabled())
            mBindingsManager->mouseWheelMoved(arg);

        input->setJoystickLastUsed(false);
    }

    void MouseManager::mousePressed(const SDL_MouseButtonEvent &arg, Uint8 id)
    {
        MWBase::Environment::get().getInputManager()->setJoystickLastUsed(false);
        bool guiMode = false;

        if (id == SDL_BUTTON_LEFT || id == SDL_BUTTON_RIGHT) // MyGUI only uses these mouse events
        {
            guiMode = MWBase::Environment::get().getWindowManager()->isGuiMode();
            guiMode = MyGUI::InputManager::getInstance().injectMousePress(static_cast<int>(mGuiCursorX), static_cast<int>(mGuiCursorY), sdlButtonToMyGUI(id)) && guiMode;
            if (MyGUI::InputManager::getInstance().getMouseFocusWidget () != 0)
            {
                MyGUI::Button* b = MyGUI::InputManager::getInstance().getMouseFocusWidget()->castType<MyGUI::Button>(false);
                if (b && b->getEnabled() && id == SDL_BUTTON_LEFT)
                {
                    MWBase::Environment::get().getWindowManager()->playSound("Menu Click");
                }
            }
            MWBase::Environment::get().getWindowManager()->setCursorActive(true);
        }

        mBindingsManager->setPlayerControlsEnabled(!guiMode);

        // Don't trigger any mouse bindings while in settings menu, otherwise rebinding controls becomes impossible
        if (MWBase::Environment::get().getWindowManager()->getMode() != MWGui::GM_Settings)
            mBindingsManager->mousePressed(arg, id);
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

        //we let the mouse escape in the main menu
        mInputWrapper->setGrabPointer(grab && (mGrabCursor || isRelative));

        //we switched to non-relative mode, move our cursor to where the in-game
        //cursor is
        if (!isRelative && wasRelative != isRelative)
        {
            warpMouse();
        }
    }

    void MouseManager::update(float dt)
    {
        if (!mMouseLookEnabled)
            return;

        float xAxis = mBindingsManager->getActionValue(A_LookLeftRight) * 2.0f - 1.0f;
        float yAxis = mBindingsManager->getActionValue(A_LookUpDown) * 2.0f - 1.0f;
        if (xAxis == 0 && yAxis == 0)
            return;

        float rot[3];
        rot[0] = -yAxis * dt * 1000.0f * mCameraSensitivity * (mInvertY ? -1 : 1) * mCameraYMultiplier / 256.f;
        rot[1] = 0.0f;
        rot[2] = -xAxis * dt * 1000.0f * mCameraSensitivity * (mInvertX ? -1 : 1) / 256.f;

        // Only actually turn player when we're not in vanity mode
        bool controls = MWBase::Environment::get().getInputManager()->getControlSwitch("playercontrols");
        if (!MWBase::Environment::get().getWorld()->vanityRotateCamera(rot) && controls)
        {
            MWWorld::Player& player = MWBase::Environment::get().getWorld()->getPlayer();
            player.yaw(-rot[2]);
            player.pitch(-rot[0]);
        }
        else if (!controls)
            MWBase::Environment::get().getWorld()->disableDeferredPreviewRotation();

        MWBase::Environment::get().getInputManager()->resetIdleTime();
    }

    bool MouseManager::injectMouseButtonPress(Uint8 button)
    {
        return MyGUI::InputManager::getInstance().injectMousePress(static_cast<int>(mGuiCursorX), static_cast<int>(mGuiCursorY), sdlButtonToMyGUI(button));
    }

    bool MouseManager::injectMouseButtonRelease(Uint8 button)
    {
        return MyGUI::InputManager::getInstance().injectMouseRelease(static_cast<int>(mGuiCursorX), static_cast<int>(mGuiCursorY), sdlButtonToMyGUI(button));
    }

    void MouseManager::injectMouseMove(float xMove, float yMove, float mouseWheelMove)
    {
        mGuiCursorX += xMove;
        mGuiCursorY += yMove;
        mMouseWheel += mouseWheelMove;

        const MyGUI::IntSize& viewSize = MyGUI::RenderManager::getInstance().getViewSize();
        mGuiCursorX = std::max(0.f, std::min(mGuiCursorX, float(viewSize.width - 1)));
        mGuiCursorY = std::max(0.f, std::min(mGuiCursorY, float(viewSize.height - 1)));

        MyGUI::InputManager::getInstance().injectMouseMove(static_cast<int>(mGuiCursorX), static_cast<int>(mGuiCursorY), static_cast<int>(mMouseWheel));
    }

    void MouseManager::warpMouse()
    {
        mInputWrapper->warpMouse(static_cast<int>(mGuiCursorX / mInvUiScalingFactor), static_cast<int>(mGuiCursorY / mInvUiScalingFactor));
    }
}
