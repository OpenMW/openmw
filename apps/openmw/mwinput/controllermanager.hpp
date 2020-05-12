#ifndef MWINPUT_MWCONTROLLERMANAGER_H
#define MWINPUT_MWCONTROLLERMANAGER_H

#include <string>

#include <components/settings/settings.hpp>
#include <components/sdlutil/events.hpp>

namespace MWInput
{
    class ActionManager;
    class BindingsManager;
    class MouseManager;

    class ControllerManager : public SDLUtil::ControllerListener
    {
    public:
        ControllerManager(BindingsManager* bindingsManager,
            ActionManager* actionManager,
            MouseManager* mouseManager,
            const std::string& userControllerBindingsFile,
            const std::string& controllerBindingsFile);

        virtual ~ControllerManager() = default;

        bool update(float dt, bool disableControls);

        virtual void buttonPressed(int deviceID, const SDL_ControllerButtonEvent &arg);
        virtual void buttonReleased(int deviceID, const SDL_ControllerButtonEvent &arg);
        virtual void axisMoved(int deviceID, const SDL_ControllerAxisEvent &arg);
        virtual void controllerAdded(int deviceID, const SDL_ControllerDeviceEvent &arg);
        virtual void controllerRemoved(const SDL_ControllerDeviceEvent &arg);

        void processChangedSettings(const Settings::CategorySettingVector& changed);

        void setJoystickLastUsed(bool enabled) { mJoystickLastUsed = enabled; }
        bool joystickLastUsed() { return mJoystickLastUsed; }

        void setGuiCursorEnabled(bool enabled) { mGuiCursorEnabled = enabled; }

        void setGamepadGuiCursorEnabled(bool enabled) { mGamepadGuiCursorEnabled = enabled; }
        bool gamepadGuiCursorEnabled() { return mGamepadGuiCursorEnabled; }

    private:
        // Return true if GUI consumes input.
        bool gamepadToGuiControl(const SDL_ControllerButtonEvent &arg);
        bool gamepadToGuiControl(const SDL_ControllerAxisEvent &arg);

        BindingsManager* mBindingsManager;
        ActionManager* mActionManager;
        MouseManager* mMouseManager;

        bool mJoystickEnabled;
        float mGamepadCursorSpeed;
        float mInvUiScalingFactor;
        float mSneakToggleShortcutTimer;
        float mGamepadZoom;
        bool mGamepadGuiCursorEnabled;
        bool mControlsDisabled;
        bool mJoystickLastUsed;
        bool mGuiCursorEnabled;
        bool mSneakGamepadShortcut;
        bool mGamepadPreviewMode;
    };
}
#endif
