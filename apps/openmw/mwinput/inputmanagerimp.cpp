#include "inputmanagerimp.hpp"

#include <osgViewer/ViewerEventHandlers>

#include <components/sdlutil/sdlinputwrapper.hpp>
#include <components/esm/esmwriter.hpp>
#include <components/esm/esmreader.hpp>
#include <components/esm/controlsstate.hpp>

#include "../mwbase/windowmanager.hpp"
#include "../mwbase/environment.hpp"

#include "../mwworld/esmstore.hpp"

#include "actionmanager.hpp"
#include "bindingsmanager.hpp"
#include "controllermanager.hpp"
#include "controlswitch.hpp"
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
        : mGrabCursor(Settings::Manager::getBool("grab cursor", "Input"))
    {
        mInputWrapper = new SDLUtil::InputWrapper(window, viewer, grab);
        mInputWrapper->setWindowEventCallback(MWBase::Environment::get().getWindowManager());

        mBindingsManager = new BindingsManager(userFile, userFileExists);

        mControlSwitch = new ControlSwitch();

        mActionManager = new ActionManager(mBindingsManager, screenCaptureOperation, viewer, screenCaptureHandler);

        mKeyboardManager = new KeyboardManager(mBindingsManager);
        mInputWrapper->setKeyboardEventCallback(mKeyboardManager);

        mMouseManager = new MouseManager(mBindingsManager, mInputWrapper, window);
        mInputWrapper->setMouseEventCallback(mMouseManager);

        mControllerManager = new ControllerManager(mBindingsManager, mActionManager, mMouseManager, userControllerBindingsFile, controllerBindingsFile);
        mInputWrapper->setControllerEventCallback(mControllerManager);

        mSensorManager = new SensorManager();
        mInputWrapper->setSensorEventCallback(mSensorManager);
    }

    void InputManager::clear()
    {
        // Enable all controls
        mControlSwitch->clear();
    }

    InputManager::~InputManager()
    {
        delete mActionManager;
        delete mControllerManager;
        delete mKeyboardManager;
        delete mMouseManager;
        delete mSensorManager;

        delete mControlSwitch;

        delete mBindingsManager;

        delete mInputWrapper;
    }

    void InputManager::setAttemptJump(bool jumping)
    {
        mActionManager->setAttemptJump(jumping);
    }

    void InputManager::updateCursorMode()
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
            mMouseManager->warpMouse();
        }
    }

    void InputManager::update(float dt, bool disableControls, bool disableEvents)
    {
        mInputWrapper->setMouseVisible(MWBase::Environment::get().getWindowManager()->getCursorVisible());
        mInputWrapper->capture(disableEvents);

        mKeyboardManager->setControlsDisabled(disableControls);
        if (disableControls)
        {
            updateCursorMode();
            return;
        }

        mBindingsManager->update(dt);

        updateCursorMode();

        bool controllerMove = mControllerManager->update(dt, disableControls);
        mMouseManager->update(dt, disableControls);
        mSensorManager->update(dt);
        mActionManager->update(dt, controllerMove);
    }

    void InputManager::setDragDrop(bool dragDrop)
    {
        mBindingsManager->setDragDrop(dragDrop);
    }

    void InputManager::setGamepadGuiCursorEnabled(bool enabled)
    {
        mControllerManager->setGamepadGuiCursorEnabled(enabled);
    }

    void InputManager::changeInputMode(bool guiMode)
    {
        mControllerManager->setGuiCursorEnabled(guiMode);
        mMouseManager->setGuiCursorEnabled(guiMode);
        mSensorManager->setGuiCursorEnabled(guiMode);
        mMouseManager->setMouseLookEnabled(!guiMode);
        if (guiMode)
            MWBase::Environment::get().getWindowManager()->showCrosshair(false);

        bool isCursorVisible = guiMode && (!mControllerManager->joystickLastUsed() || mControllerManager->gamepadGuiCursorEnabled());
        MWBase::Environment::get().getWindowManager()->setCursorVisible(isCursorVisible);
        // if not in gui mode, the camera decides whether to show crosshair or not.
    }

    void InputManager::processChangedSettings(const Settings::CategorySettingVector& changed)
    {
        for (const auto& setting : changed)
        {
            if (setting.first == "Input" && setting.second == "grab cursor")
                mGrabCursor = Settings::Manager::getBool("grab cursor", "Input");
        }

        mMouseManager->processChangedSettings(changed);
        mSensorManager->processChangedSettings(changed);
    }

    bool InputManager::getControlSwitch(const std::string& sw)
    {
        return mControlSwitch->get(sw);
    }

    void InputManager::toggleControlSwitch(const std::string& sw, bool value)
    {
        mControlSwitch->set(sw, value);
    }

    void InputManager::resetIdleTime()
    {
        mActionManager->resetIdleTime();
    }

    std::string InputManager::getActionDescription(int action)
    {
        return mBindingsManager->getActionDescription(action);
    }

    std::string InputManager::getActionKeyBindingName(int action)
    {
        return mBindingsManager->getActionKeyBindingName(action);
    }

    std::string InputManager::getActionControllerBindingName(int action)
    {
        return mBindingsManager->getActionControllerBindingName(action);
    }

    std::vector<int> InputManager::getActionKeySorting()
    {
        return mBindingsManager->getActionKeySorting();
    }

    std::vector<int> InputManager::getActionControllerSorting()
    {
        return mBindingsManager->getActionControllerSorting();
    }

    void InputManager::enableDetectingBindingMode(int action, bool keyboard)
    {
        mBindingsManager->enableDetectingBindingMode(action, keyboard);
    }

    int InputManager::countSavedGameRecords() const
    {
        return mControlSwitch->countSavedGameRecords();
    }

    void InputManager::write(ESM::ESMWriter& writer, Loading::Listener& progress)
    {
        mControlSwitch->write(writer, progress);
    }

    void InputManager::readRecord(ESM::ESMReader& reader, uint32_t type)
    {
        if (type == ESM::REC_INPU)
        {
            mControlSwitch->readRecord(reader, type);
        }
    }

    void InputManager::resetToDefaultKeyBindings()
    {
        mBindingsManager->loadKeyDefaults(true);
    }

    void InputManager::resetToDefaultControllerBindings()
    {
        mBindingsManager->loadControllerDefaults(true);
    }

    void InputManager::setJoystickLastUsed(bool enabled)
    {
        mControllerManager->setJoystickLastUsed(enabled);
    }

    bool InputManager::joystickLastUsed()
    {
        return mControllerManager->joystickLastUsed();
    }

    void InputManager::executeAction(int action)
    {
        mActionManager->executeAction(action);
    }
}
