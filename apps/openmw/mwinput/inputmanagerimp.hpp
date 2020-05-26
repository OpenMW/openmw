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
        virtual void clear();

        virtual void update(float dt, bool disableControls=false, bool disableEvents=false);

        virtual void changeInputMode(bool guiMode);

        virtual void processChangedSettings(const Settings::CategorySettingVector& changed);

        virtual void setDragDrop(bool dragDrop);
        virtual void setGamepadGuiCursorEnabled(bool enabled);
        virtual void setAttemptJump(bool jumping);

        virtual void toggleControlSwitch (const std::string& sw, bool value);
        virtual bool getControlSwitch (const std::string& sw);

        virtual std::string getActionDescription (int action);
        virtual std::string getActionKeyBindingName (int action);
        virtual std::string getActionControllerBindingName (int action);
        virtual int getNumActions() { return A_Last; }
        virtual std::vector<int> getActionKeySorting();
        virtual std::vector<int> getActionControllerSorting();
        virtual void enableDetectingBindingMode (int action, bool keyboard);
        virtual void resetToDefaultKeyBindings();
        virtual void resetToDefaultControllerBindings();

        virtual void setJoystickLastUsed(bool enabled);
        virtual bool joystickLastUsed();

        virtual int countSavedGameRecords() const;
        virtual void write(ESM::ESMWriter& writer, Loading::Listener& progress);
        virtual void readRecord(ESM::ESMReader& reader, uint32_t type);

        virtual void resetIdleTime();

        virtual void executeAction(int action);

        virtual bool controlsDisabled() { return mControlsDisabled; }

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
