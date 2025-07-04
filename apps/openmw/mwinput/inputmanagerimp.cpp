#include "inputmanagerimp.hpp"

#include <osgViewer/ViewerEventHandlers>

#include <components/esm3/esmreader.hpp>
#include <components/esm3/esmwriter.hpp>
#include <components/sdlutil/sdlinputwrapper.hpp>
#include <components/settings/values.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/esmstore.hpp"

#include "actionmanager.hpp"
#include "bindingsmanager.hpp"
#include "controllermanager.hpp"
#include "controlswitch.hpp"
#include "gyromanager.hpp"
#include "keyboardmanager.hpp"
#include "mousemanager.hpp"
#include "sensormanager.hpp"

namespace MWInput
{
    InputManager::InputManager(SDL_Window* window, osg::ref_ptr<osgViewer::Viewer> viewer,
        osg::ref_ptr<osgViewer::ScreenCaptureHandler> screenCaptureHandler, const std::filesystem::path& userFile,
        bool userFileExists, const std::filesystem::path& userControllerBindingsFile,
        const std::filesystem::path& controllerBindingsFile, bool grab)
        : mControlsDisabled(false)
        , mInputWrapper(std::make_unique<SDLUtil::InputWrapper>(window, viewer, grab))
        , mBindingsManager(std::make_unique<BindingsManager>(userFile, userFileExists))
        , mControlSwitch(std::make_unique<ControlSwitch>())
        , mActionManager(std::make_unique<ActionManager>(mBindingsManager.get(), viewer, screenCaptureHandler))
        , mKeyboardManager(std::make_unique<KeyboardManager>(mBindingsManager.get()))
        , mMouseManager(std::make_unique<MouseManager>(mBindingsManager.get(), mInputWrapper.get(), window))
        , mControllerManager(std::make_unique<ControllerManager>(
              mBindingsManager.get(), mMouseManager.get(), userControllerBindingsFile, controllerBindingsFile))
        , mSensorManager(std::make_unique<SensorManager>())
        , mGyroManager(std::make_unique<GyroManager>())
    {
        mInputWrapper->setWindowEventCallback(MWBase::Environment::get().getWindowManager());
        mInputWrapper->setKeyboardEventCallback(mKeyboardManager.get());
        mInputWrapper->setMouseEventCallback(mMouseManager.get());
        mInputWrapper->setControllerEventCallback(mControllerManager.get());
        mInputWrapper->setSensorEventCallback(mSensorManager.get());
    }

    void InputManager::clear()
    {
        // Enable all controls
        mControlSwitch->clear();
    }

    InputManager::~InputManager() {}

    void InputManager::update(float dt, bool disableControls, bool disableEvents)
    {
        mControlsDisabled = disableControls;

        mInputWrapper->setMouseVisible(MWBase::Environment::get().getWindowManager()->getCursorVisible());
        mInputWrapper->capture(disableEvents);

        if (disableControls)
        {
            mMouseManager->updateCursorMode();
            return;
        }

        mBindingsManager->update(dt);

        mMouseManager->updateCursorMode();

        mControllerManager->update(dt);
        mMouseManager->update(dt);
        mSensorManager->update(dt);
        mActionManager->update(dt);

        if (Settings::input().mEnableGyroscope)
        {
            bool controllerAvailable = mControllerManager->isGyroAvailable();
            bool sensorAvailable = mSensorManager->isGyroAvailable();
            if (controllerAvailable || sensorAvailable)
            {
                mGyroManager->update(
                    dt, controllerAvailable ? mControllerManager->getGyroValues() : mSensorManager->getGyroValues());
            }
        }
    }

    void InputManager::setDragDrop(bool dragDrop)
    {
        mBindingsManager->setDragDrop(dragDrop);
    }

    void InputManager::setGamepadGuiCursorEnabled(bool enabled)
    {
        mControllerManager->setGamepadGuiCursorEnabled(enabled);
    }

    bool InputManager::isGamepadGuiCursorEnabled()
    {
        return mControllerManager->gamepadGuiCursorEnabled();
    }

    void InputManager::changeInputMode(bool guiMode)
    {
        mControllerManager->setGuiCursorEnabled(guiMode);
        mMouseManager->setGuiCursorEnabled(guiMode);
        mGyroManager->setGuiCursorEnabled(guiMode);
        mMouseManager->setMouseLookEnabled(!guiMode);
        if (guiMode)
            MWBase::Environment::get().getWindowManager()->showCrosshair(false);

        bool isCursorVisible
            = guiMode && (!mControllerManager->joystickLastUsed() || mControllerManager->gamepadGuiCursorEnabled());
        MWBase::Environment::get().getWindowManager()->setCursorVisible(isCursorVisible);
        // if not in gui mode, the camera decides whether to show crosshair or not.
    }

    void InputManager::processChangedSettings(const Settings::CategorySettingVector& changed)
    {
        mSensorManager->processChangedSettings(changed);
    }

    bool InputManager::getControlSwitch(std::string_view sw)
    {
        return mControlSwitch->get(sw);
    }

    void InputManager::toggleControlSwitch(std::string_view sw, bool value)
    {
        mControlSwitch->set(sw, value);
    }

    void InputManager::resetIdleTime()
    {
        mActionManager->resetIdleTime();
    }

    bool InputManager::isIdle() const
    {
        return mActionManager->getIdleTime() > 0.5;
    }

    std::string_view InputManager::getActionDescription(int action) const
    {
        return mBindingsManager->getActionDescription(action);
    }

    std::string InputManager::getActionKeyBindingName(int action) const
    {
        return mBindingsManager->getActionKeyBindingName(action);
    }

    std::string InputManager::getActionControllerBindingName(int action) const
    {
        return mBindingsManager->getActionControllerBindingName(action);
    }

    bool InputManager::actionIsActive(int action) const
    {
        return mBindingsManager->actionIsActive(action);
    }

    float InputManager::getActionValue(int action) const
    {
        return mBindingsManager->getActionValue(action);
    }

    bool InputManager::isControllerButtonPressed(SDL_GameControllerButton button) const
    {
        return mControllerManager->isButtonPressed(button);
    }

    float InputManager::getControllerAxisValue(SDL_GameControllerAxis axis) const
    {
        return mControllerManager->getAxisValue(axis);
    }

    int InputManager::getMouseMoveX() const
    {
        return mMouseManager->getMouseMoveX();
    }

    int InputManager::getMouseMoveY() const
    {
        return mMouseManager->getMouseMoveY();
    }

    void InputManager::warpMouseToWidget(MyGUI::Widget* widget)
    {
        mMouseManager->warpMouseToWidget(widget);
        mMouseManager->injectMouseMove(1, 0, 0);
        MWBase::Environment::get().getWindowManager()->setCursorActive(true);
    }

    const std::initializer_list<int>& InputManager::getActionKeySorting()
    {
        return mBindingsManager->getActionKeySorting();
    }

    const std::initializer_list<int>& InputManager::getActionControllerSorting()
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

    std::string InputManager::getControllerButtonIcon(int button)
    {
        return mControllerManager->getControllerButtonIcon(button);
    }

    std::string InputManager::getControllerAxisIcon(int axis)
    {
        return mControllerManager->getControllerAxisIcon(axis);
    }

    void InputManager::executeAction(int action)
    {
        mActionManager->executeAction(action);
    }
}
