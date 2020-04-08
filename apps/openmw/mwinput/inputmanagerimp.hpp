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
    class VideoWrapper;
}

namespace osgViewer
{
    class Viewer;
    class ScreenCaptureHandler;
}

struct SDL_Window;

namespace MWInput
{
    const float ZOOM_SCALE = 120.f; /// Used for scrolling camera in and out

    /**
    * @brief Class that handles all input and key bindings for OpenMW.
    */
    class InputManager :
            public MWBase::InputManager,
            public SDLUtil::KeyListener,
            public SDLUtil::MouseListener,
            public SDLUtil::WindowListener,
            public SDLUtil::ControllerListener,
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

        virtual bool isWindowVisible();

        /// Clear all savegame-specific data
        virtual void clear();

        virtual void update(float dt, bool disableControls=false, bool disableEvents=false);

        void setPlayer (MWWorld::Player* player);

        virtual void changeInputMode(bool guiMode);

        virtual void processChangedSettings(const Settings::CategorySettingVector& changed);

        virtual void setDragDrop(bool dragDrop);

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

        virtual bool joystickLastUsed() {return mJoystickLastUsed;}

        virtual void keyPressed(const SDL_KeyboardEvent &arg );
        virtual void keyReleased( const SDL_KeyboardEvent &arg );
        virtual void textInput (const SDL_TextInputEvent &arg);

        virtual void mousePressed( const SDL_MouseButtonEvent &arg, Uint8 id );
        virtual void mouseReleased( const SDL_MouseButtonEvent &arg, Uint8 id );
        virtual void mouseMoved( const SDLUtil::MouseMotionEvent &arg );

        virtual void mouseWheelMoved( const SDL_MouseWheelEvent &arg);

        virtual void buttonPressed(int deviceID, const SDL_ControllerButtonEvent &arg);
        virtual void buttonReleased(int deviceID, const SDL_ControllerButtonEvent &arg);
        virtual void axisMoved(int deviceID, const SDL_ControllerAxisEvent &arg);
        virtual void controllerAdded(int deviceID, const SDL_ControllerDeviceEvent &arg);
        virtual void controllerRemoved(const SDL_ControllerDeviceEvent &arg);

        virtual void windowVisibilityChange( bool visible );
        virtual void windowFocusChange( bool have_focus );
        virtual void windowResized (int x, int y);
        virtual void windowClosed ();

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

    private:
        SDL_Window* mWindow;
        bool mWindowVisible;
        osg::ref_ptr<osgViewer::Viewer> mViewer;
        osg::ref_ptr<osgViewer::ScreenCaptureHandler> mScreenCaptureHandler;
        osgViewer::ScreenCaptureHandler::CaptureOperation *mScreenCaptureOperation;

        bool mJoystickLastUsed;
        MWWorld::Player* mPlayer;

        ICS::InputControlSystem* mInputBinder;

        SDLUtil::InputWrapper* mInputManager;
        SDLUtil::VideoWrapper* mVideoWrapper;

        std::string mUserFile;

        bool mDragDrop;

        bool mGrabCursor;

        bool mInvertX;
        bool mInvertY;

        bool mControlsDisabled;
        bool mJoystickEnabled;

        float mCameraSensitivity;
        float mCameraYMultiplier;
        float mPreviewPOVDelay;
        float mTimeIdle;

        bool mMouseLookEnabled;
        bool mGuiCursorEnabled;
        bool mGamepadGuiCursorEnabled;

        bool mDetectingKeyboard;

        float mOverencumberedMessageDelay;

        float mGuiCursorX;
        float mGuiCursorY;
        int mMouseWheel;
        float mGamepadZoom;
        bool mUserFileExists;
        bool mAlwaysRunActive;
        bool mSneakToggles;
        float mSneakToggleShortcutTimer;
        bool mSneakGamepadShortcut;
        bool mSneaking;
        bool mAttemptJump;

        std::map<std::string, bool> mControlSwitch;

        float mInvUiScalingFactor;
        float mGamepadCursorSpeed;

        SensorManager* mSensorManager;

        void convertMousePosForMyGUI(int& x, int& y);

        void resetIdleTime();
        void updateIdleTime(float dt);

        void setPlayerControlsEnabled(bool enabled);
        void handleGuiArrowKey(int action);
        // Return true if GUI consumes input.
        bool gamepadToGuiControl(const SDL_ControllerButtonEvent &arg);
        bool gamepadToGuiControl(const SDL_ControllerAxisEvent &arg);

        void updateCursorMode();

        bool checkAllowedToUseItems() const;

        void toggleMainMenu();
        void toggleSpell();
        void toggleWeapon();
        void toggleInventory();
        void toggleConsole();
        void screenshot();
        void toggleJournal();
        void activate();
        void toggleWalking();
        void toggleSneaking();
        void toggleAutoMove();
        void rest();
        void quickLoad();
        void quickSave();

        void quickKey (int index);
        void showQuickKeysMenu();

        bool actionIsActive (int id);

        void loadKeyDefaults(bool force = false);
        void loadControllerDefaults(bool force = false);

        int mFakeDeviceID; //As we only support one controller at a time, use a fake deviceID so we don't lose bindings when switching controllers
    };
}
#endif
