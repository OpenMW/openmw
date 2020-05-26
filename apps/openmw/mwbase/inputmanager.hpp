#ifndef GAME_MWBASE_INPUTMANAGER_H
#define GAME_MWBASE_INPUTMANAGER_H

#include <string>
#include <set>
#include <vector>

#include <stdint.h>

namespace Loading
{
    class Listener;
}

namespace ESM
{
    class ESMReader;
    class ESMWriter;
}

namespace MWBase
{
    /// \brief Interface for input manager (implemented in MWInput)
    class InputManager
    {
            InputManager (const InputManager&);
            ///< not implemented

            InputManager& operator= (const InputManager&);
            ///< not implemented

        public:

            InputManager() {}

            /// Clear all savegame-specific data
            virtual void clear() = 0;

            virtual ~InputManager() {}

            virtual void update(float dt, bool disableControls, bool disableEvents=false) = 0;

            virtual void changeInputMode(bool guiMode) = 0;

            virtual void processChangedSettings(const std::set< std::pair<std::string, std::string> >& changed) = 0;

            virtual void setDragDrop(bool dragDrop) = 0;
            virtual void setGamepadGuiCursorEnabled(bool enabled) = 0;
            virtual void setAttemptJump(bool jumping) = 0;

            virtual void toggleControlSwitch (const std::string& sw, bool value) = 0;
            virtual bool getControlSwitch (const std::string& sw) = 0;

            virtual std::string getActionDescription (int action) = 0;
            virtual std::string getActionKeyBindingName (int action) = 0;
            virtual std::string getActionControllerBindingName (int action) = 0;
            ///Actions available for binding to keyboard buttons
            virtual std::vector<int> getActionKeySorting() = 0;
            ///Actions available for binding to controller buttons
            virtual std::vector<int> getActionControllerSorting() = 0;
            virtual int getNumActions() = 0;
            ///If keyboard is true, only pay attention to keyboard events. If false, only pay attention to controller events (excluding esc)
            virtual void enableDetectingBindingMode (int action, bool keyboard) = 0;
            virtual void resetToDefaultKeyBindings() = 0;
            virtual void resetToDefaultControllerBindings() = 0;

            /// Returns if the last used input device was a joystick or a keyboard
            /// @return true if joystick, false otherwise
            virtual bool joystickLastUsed() = 0;
            virtual void setJoystickLastUsed(bool enabled) = 0;

            virtual int countSavedGameRecords() const = 0;
            virtual void write(ESM::ESMWriter& writer, Loading::Listener& progress) = 0;
            virtual void readRecord(ESM::ESMReader& reader, uint32_t type) = 0;

            virtual void resetIdleTime() = 0;

            virtual void executeAction(int action) = 0;

            virtual bool controlsDisabled() = 0;
    };
}

#endif
