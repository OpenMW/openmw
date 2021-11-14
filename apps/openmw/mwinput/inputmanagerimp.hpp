#ifndef MWINPUT_MWINPUTMANAGERIMP_H
#define MWINPUT_MWINPUTMANAGERIMP_H

#include <osg/ref_ptr>
#include <osgViewer/ViewerEventHandlers>

#include <components/settings/settings.hpp>
#include <components/sdlutil/events.hpp>

#include "../mwbase/inputmanager.hpp"

#include "../mwgui/mode.hpp"

#include "actions.hpp"

namespace MWWorld
{
    class Player;
}

namespace MWBase
{
    class WindowManager;
}

namespace SDLUtil
{
    class InputWrapper;
}

struct SDL_Window;

namespace MWInput
{
    class ControlSwitch;
    class ActionManager;
    class BindingsManager;
    class ControllerManager;
    class KeyboardManager;
    class MouseManager;
    class SensorManager;

    /**
    * @brief Class that provides a high-level API for game input
    */
    class InputManager : public MWBase::InputManager
    {
    public:
        InputManager(
            SDL_Window* window,
            osg::ref_ptr<osgViewer::Viewer> viewer,
            osg::ref_ptr<osgViewer::ScreenCaptureHandler> screenCaptureHandler,
            osgViewer::ScreenCaptureHandler::CaptureOperation *screenCaptureOperation,
            const std::string& userFile, bool userFileExists,
            const std::string& userControllerBindingsFile,
            const std::string& controllerBindingsFile, bool grab);

        virtual ~InputManager();

        /// Clear all savegame-specific data
        void clear() override;

        void update(float dt, bool disableControls=false, bool disableEvents=false) override;

        void changeInputMode(bool guiMode) override;

        void processChangedSettings(const Settings::CategorySettingVector& changed) override;

        void setDragDrop(bool dragDrop) override;
        void setGamepadGuiCursorEnabled(bool enabled) override;
        void setAttemptJump(bool jumping) override;

        void toggleControlSwitch(std::string_view sw, bool value) override;
        bool getControlSwitch(std::string_view sw) override;

        std::string getActionDescription (int action) const override;
        std::string getActionKeyBindingName (int action) const override;
        std::string getActionControllerBindingName (int action) const override;
        bool actionIsActive(int action) const override;

        float getActionValue(int action) const override;
        bool isControllerButtonPressed(SDL_GameControllerButton button) const override;
        float getControllerAxisValue(SDL_GameControllerAxis axis) const override;
        int getMouseMoveX() const override;
        int getMouseMoveY() const override;

        int getNumActions() override { return A_Last; }
        std::vector<int> getActionKeySorting() override;
        std::vector<int> getActionControllerSorting() override;
        void enableDetectingBindingMode (int action, bool keyboard) override;
        void resetToDefaultKeyBindings() override;
        void resetToDefaultControllerBindings() override;

        void setJoystickLastUsed(bool enabled) override;
        bool joystickLastUsed() override;

        int countSavedGameRecords() const override;
        void write(ESM::ESMWriter& writer, Loading::Listener& progress) override;
        void readRecord(ESM::ESMReader& reader, uint32_t type) override;

        void resetIdleTime() override;
        bool isIdle() const override;

        void executeAction(int action) override;

        bool controlsDisabled() override { return mControlsDisabled; }

    private:
        void convertMousePosForMyGUI(int& x, int& y);

        void handleGuiArrowKey(int action);

        void quickKey(int index);
        void showQuickKeysMenu();

        void loadKeyDefaults(bool force = false);
        void loadControllerDefaults(bool force = false);

        SDLUtil::InputWrapper* mInputWrapper;

        bool mControlsDisabled;

        ControlSwitch* mControlSwitch;

        ActionManager* mActionManager;
        BindingsManager* mBindingsManager;
        ControllerManager* mControllerManager;
        KeyboardManager* mKeyboardManager;
        MouseManager* mMouseManager;
        SensorManager* mSensorManager;
    };
}
#endif
