#ifndef MWINPUT_MWBINDINGSMANAGER_H
#define MWINPUT_MWBINDINGSMANAGER_H

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <components/sdlutil/events.hpp>

namespace MWInput
{
    class BindingsListener;
    class InputControlSystem;

    class BindingsManager
    {
    public:
        BindingsManager(const std::filesystem::path& userFile, bool userFileExists);

        virtual ~BindingsManager();

        std::string_view getActionDescription(int action);
        std::string getActionKeyBindingName(int action);
        std::string getActionControllerBindingName(int action);
        const std::initializer_list<int>& getActionKeySorting();
        const std::initializer_list<int>& getActionControllerSorting();

        void enableDetectingBindingMode(int action, bool keyboard);
        bool isDetectingBindingState() const;

        void loadKeyDefaults(bool force = false);
        void loadControllerDefaults(bool force = false);

        void setDragDrop(bool dragDrop);

        void update(float dt);

        void setPlayerControlsEnabled(bool enabled);

        void setJoystickDeadZone(float deadZone);

        bool isLeftOrRightButton(int action, bool joystick) const;

        bool actionIsActive(int id) const;
        float getActionValue(int id) const; // returns value in range [0, 1]

        SDL_GameController* getControllerOrNull() const;

        void mousePressed(const SDL_MouseButtonEvent& evt, Uint8 deviceID);
        void mouseReleased(const SDL_MouseButtonEvent& arg, Uint8 deviceID);
        void mouseMoved(const SDLUtil::MouseMotionEvent& arg);
        void mouseWheelMoved(const SDL_MouseWheelEvent& arg);

        void keyPressed(const SDL_KeyboardEvent& arg);
        void keyReleased(const SDL_KeyboardEvent& arg);

        void controllerAdded(int deviceID, const SDL_ControllerDeviceEvent& arg);
        void controllerRemoved(const SDL_ControllerDeviceEvent& arg);
        void controllerButtonPressed(int deviceID, const SDL_ControllerButtonEvent& arg);
        void controllerButtonReleased(int deviceID, const SDL_ControllerButtonEvent& arg);
        void controllerAxisMoved(int deviceID, const SDL_ControllerAxisEvent& arg);

        SDL_Scancode getKeyBinding(int actionId);

        void actionValueChanged(int action, float currentValue, float previousValue);

        void saveBindings();

    private:
        void setupSDLKeyMappings();

        std::unique_ptr<InputControlSystem> mInputBinder;
        std::unique_ptr<BindingsListener> mListener;

        std::filesystem::path mUserFile;

        bool mDragDrop;
    };
}
#endif
