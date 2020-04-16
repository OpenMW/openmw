#ifndef MWINPUT_MWINPUTMANAGERIMP_H
#define MWINPUT_MWINPUTMANAGERIMP_H

#include "../mwgui/mode.hpp"

#include <SDL_sensor.h>

#include <osg/ref_ptr>
#include <osgViewer/ViewerEventHandlers>

#include <extern/oics/ICSChannelListener.h>
#include <extern/oics/ICSInputControlSystem.h>

#include <components/settings/settings.hpp>
#include <components/files/configurationmanager.hpp>
#include <components/sdlutil/events.hpp>

#include "../mwbase/inputmanager.hpp"

#include "actions.hpp"

namespace MWInput
{
    class ActionManager;
    class ControllerManager;
    class KeyboardManager;
    class MouseManager;
    class SensorManager;
}

namespace MWWorld
{
    class Player;
}

namespace MWBase
{
    class WindowManager;
}

namespace ICS
{
    class InputControlSystem;
}

namespace Files
{
    struct ConfigurationManager;
}

namespace SDLUtil
{
    class InputWrapper;
}

struct SDL_Window;

namespace MWInput
{
    /**
    * @brief Class that handles all input and key bindings for OpenMW.
    */
    class InputManager :
            public MWBase::InputManager,
            public ICS::ChannelListener,
            public ICS::DetectingBindingListener
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

        virtual void channelChanged(ICS::Channel* channel, float currentValue, float previousValue);

        virtual void keyBindingDetected(ICS::InputControlSystem* ICS, ICS::Control* control
            , SDL_Scancode key, ICS::Control::ControlChangingDirection direction);

        virtual void mouseAxisBindingDetected(ICS::InputControlSystem* ICS, ICS::Control* control
            , ICS::InputControlSystem::NamedAxis axis, ICS::Control::ControlChangingDirection direction);

        virtual void mouseButtonBindingDetected(ICS::InputControlSystem* ICS, ICS::Control* control
            , unsigned int button, ICS::Control::ControlChangingDirection direction);

        virtual void mouseWheelBindingDetected(ICS::InputControlSystem* ICS, ICS::Control* control
            , ICS::InputControlSystem::MouseWheelClick click, ICS::Control::ControlChangingDirection direction);

        virtual void joystickAxisBindingDetected(ICS::InputControlSystem* ICS, int deviceID, ICS::Control* control
            , int axis, ICS::Control::ControlChangingDirection direction);

        virtual void joystickButtonBindingDetected(ICS::InputControlSystem* ICS, int deviceID, ICS::Control* control
            , unsigned int button, ICS::Control::ControlChangingDirection direction);

        void clearAllKeyBindings (ICS::Control* control);
        void clearAllControllerBindings (ICS::Control* control);

        virtual int countSavedGameRecords() const;
        virtual void write(ESM::ESMWriter& writer, Loading::Listener& progress);
        virtual void readRecord(ESM::ESMReader& reader, uint32_t type);

        virtual void setPlayerControlsEnabled(bool enabled);

        virtual void resetIdleTime();

    private:
        ICS::InputControlSystem* mInputBinder;

        SDLUtil::InputWrapper* mInputWrapper;

        std::string mUserFile;

        bool mDragDrop;

        bool mGrabCursor;

        bool mGuiCursorEnabled;

        bool mDetectingKeyboard;

        std::map<std::string, bool> mControlSwitch;

        ActionManager* mActionManager;
        ControllerManager* mControllerManager;
        KeyboardManager* mKeyboardManager;
        MouseManager* mMouseManager;
        SensorManager* mSensorManager;

        void convertMousePosForMyGUI(int& x, int& y);

        void handleGuiArrowKey(int action);

        void updateCursorMode();

        void quickKey (int index);
        void showQuickKeysMenu();

        bool actionIsActive (int id);

        void loadKeyDefaults(bool force = false);
        void loadControllerDefaults(bool force = false);

        int mFakeDeviceID; //As we only support one controller at a time, use a fake deviceID so we don't lose bindings when switching controllers
    };
}
#endif
