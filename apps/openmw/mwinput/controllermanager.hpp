#ifndef MWINPUT_MWCONTROLLERMANAGER_H
#define MWINPUT_MWCONTROLLERMANAGER_H

#include <array>
#include <filesystem>
#include <string>

#include <components/sdlutil/events.hpp>
#include <components/settings/settings.hpp>

namespace MWInput
{
    class BindingsManager;
    class MouseManager;

    class ControllerManager : public SDLUtil::ControllerListener
    {
    public:
        ControllerManager(BindingsManager* bindingsManager, MouseManager* mouseManager,
            const std::filesystem::path& userControllerBindingsFile,
            const std::filesystem::path& controllerBindingsFile);

        virtual ~ControllerManager() = default;

        void update(float dt);

        void buttonPressed(int deviceID, const SDL_ControllerButtonEvent& arg) override;
        void buttonReleased(int deviceID, const SDL_ControllerButtonEvent& arg) override;
        void axisMoved(int deviceID, const SDL_ControllerAxisEvent& arg) override;
        void controllerAdded(int deviceID, const SDL_ControllerDeviceEvent& arg) override;
        void controllerRemoved(const SDL_ControllerDeviceEvent& arg) override;

        void touchpadMoved(int deviceId, const SDLUtil::TouchEvent& arg) override;
        void touchpadPressed(int deviceId, const SDLUtil::TouchEvent& arg) override;
        void touchpadReleased(int deviceId, const SDLUtil::TouchEvent& arg) override;

        void setJoystickLastUsed(bool enabled) { mJoystickLastUsed = enabled; }
        bool joystickLastUsed() const { return mJoystickLastUsed; }

        void setGuiCursorEnabled(bool enabled) { mGuiCursorEnabled = enabled; }

        void setGamepadGuiCursorEnabled(bool enabled) { mGamepadGuiCursorEnabled = enabled; }
        bool gamepadGuiCursorEnabled() const { return mGamepadGuiCursorEnabled; }

        float getAxisValue(SDL_GameControllerAxis axis) const; // returns value in range [-1, 1]
        bool isButtonPressed(SDL_GameControllerButton button) const;

        bool isGyroAvailable() const;
        std::array<float, 3> getGyroValues() const;

    private:
        // Return true if GUI consumes input.
        bool gamepadToGuiControl(const SDL_ControllerButtonEvent& arg);
        bool gamepadToGuiControl(const SDL_ControllerAxisEvent& arg);

        void enableGyroSensor();

        BindingsManager* mBindingsManager;
        MouseManager* mMouseManager;

        bool mGyroAvailable;
        bool mGamepadGuiCursorEnabled;
        bool mGuiCursorEnabled;
        bool mJoystickLastUsed;
    };
}
#endif
