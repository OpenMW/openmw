#ifndef GAME_MWBASE_INPUTMANAGER_H
#define GAME_MWBASE_INPUTMANAGER_H

#include <set>
#include <string>
#include <vector>

#include <SDL_gamecontroller.h>
#include <cstdint>

namespace Loading
{
    class Listener;
}

namespace ESM
{
    class ESMReader;
    class ESMWriter;
}

namespace MyGUI
{
    class Widget;
}

namespace MWBase
{
    /// \brief Interface for input manager (implemented in MWInput)
    class InputManager
    {
        InputManager(const InputManager&);
        ///< not implemented

        InputManager& operator=(const InputManager&);
        ///< not implemented

    public:
        InputManager() {}

        /// Clear all savegame-specific data
        virtual void clear() = 0;

        virtual ~InputManager() {}

        virtual void update(float dt, bool disableControls, bool disableEvents = false) = 0;

        virtual void changeInputMode(bool guiMode) = 0;

        virtual void processChangedSettings(const std::set<std::pair<std::string, std::string>>& changed) = 0;

        virtual void setDragDrop(bool dragDrop) = 0;
        virtual bool isGamepadGuiCursorEnabled() = 0;
        virtual void setGamepadGuiCursorEnabled(bool enabled) = 0;

        virtual void toggleControlSwitch(std::string_view sw, bool value) = 0;
        virtual bool getControlSwitch(std::string_view sw) = 0;

        virtual std::string_view getActionDescription(int action) const = 0;
        virtual std::string getActionKeyBindingName(int action) const = 0;
        virtual std::string getActionControllerBindingName(int action) const = 0;
        virtual bool actionIsActive(int action) const = 0;

        virtual float getActionValue(int action) const = 0; // returns value in range [0, 1]
        virtual bool isControllerButtonPressed(SDL_GameControllerButton button) const = 0;
        virtual float getControllerAxisValue(SDL_GameControllerAxis axis) const = 0; // returns value in range [-1, 1]
        virtual bool controllerHasRumble() const = 0;
        virtual void playControllerRumble(float lowFrequencyStrength, float highFrequencyStrength,
            float durationSeconds) = 0;
        virtual void stopControllerRumble() = 0;
        virtual int getMouseMoveX() const = 0;
        virtual int getMouseMoveY() const = 0;
        virtual void warpMouseToWidget(MyGUI::Widget* widget) = 0;

        /// Actions available for binding to keyboard buttons
        virtual const std::initializer_list<int>& getActionKeySorting() = 0;
        /// Actions available for binding to controller buttons
        virtual const std::initializer_list<int>& getActionControllerSorting() = 0;
        virtual int getNumActions() = 0;
        /// If keyboard is true, only pay attention to keyboard events. If false, only pay attention to controller
        /// events (excluding esc)
        virtual void enableDetectingBindingMode(int action, bool keyboard) = 0;
        virtual void resetToDefaultKeyBindings() = 0;
        virtual void resetToDefaultControllerBindings() = 0;

        /// Returns if the last used input device was a joystick or a keyboard
        /// @return true if joystick, false otherwise
        virtual bool joystickLastUsed() = 0;
        virtual void setJoystickLastUsed(bool enabled) = 0;
        virtual std::string getControllerButtonIcon(int button) = 0;
        virtual std::string getControllerAxisIcon(int axis) = 0;

        virtual int countSavedGameRecords() const = 0;
        virtual void write(ESM::ESMWriter& writer, Loading::Listener& progress) = 0;
        virtual void readRecord(ESM::ESMReader& reader, uint32_t type) = 0;

        virtual void resetIdleTime() = 0;
        virtual bool isIdle() const = 0;

        virtual void executeAction(int action) = 0;

        virtual bool controlsDisabled() = 0;

        virtual void saveBindings() = 0;
    };
}

#endif
