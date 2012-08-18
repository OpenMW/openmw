#ifndef GAME_MWBASE_INPUTMANAGER_H
#define GAME_MWBASE_INPUTMANAGER_H

#include <string>

#include <components/settings/settings.hpp>

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

            virtual ~InputManager() {}

            virtual void update(float duration) = 0;

            virtual void changeInputMode(bool guiMode) = 0;

            virtual void processChangedSettings(const Settings::CategorySettingVector& changed) = 0;

            virtual void setDragDrop(bool dragDrop) = 0;

            virtual void toggleControlSwitch (const std::string& sw, bool value) = 0;

            virtual void resetIdleTime() = 0;
    };
}

#endif
