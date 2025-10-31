local input = require('openmw.input')

return {

    interfaceName = 'GamepadControls',
    ---
    -- Gamepad control interface
    -- @module GamepadControls
    -- @context player
    -- @usage require('openmw.interfaces').GamepadControls
    interface = {
        --- Interface version
        -- @field [parent=#GamepadControls] #number version
        version = 2,

        --- Checks if the gamepad cursor is active. If it is active, the left stick can move the cursor, and A will be interpreted as a mouse click.
        -- @function [parent=#GamepadControls] isGamepadCursorActive
        -- @return #boolean
        isGamepadCursorActive = function()
            return input._isGamepadCursorActive()
        end,

        --- Checks if the controller menu option is enabled. If true, UI is replaced with a more controller appropriate interface.
        -- @function [parent=#GamepadControls] isControllerMenusEnabled
        -- @return #boolean
        isControllerMenusEnabled = function()
            return input._isControllerMenusEnabled()
        end,

        --- Sets if the gamepad cursor is active. If it is active, the left stick can move the cursor, and A will be interpreted as a mouse click.
        -- @function [parent=#GamepadControls] setGamepadCursorActive
        -- @param #boolean value
        setGamepadCursorActive = function(state)
            input._setGamepadCursorActive(state)
        end,

        --- Check if the connected controller supports rumble and the feature is enabled in settings.
        -- @function [parent=#GamepadControls] hasRumble
        -- @return #boolean
        hasRumble = function()
            return input.controllerHasRumble()
        end,

        --- Trigger a rumble effect on the active controller.
        -- @function [parent=#GamepadControls] startRumble
        -- @param options Table passed to @{openmw.input#controllerStartRumble}.
        -- @return #boolean ``true`` if rumble started, ``false`` otherwise.
        startRumble = function(options)
            return input.controllerStartRumble(options)
        end,

        --- Stop any active controller rumble effect.
        -- @function [parent=#GamepadControls] stopRumble
        stopRumble = function()
            input.controllerStopRumble()
        end,
    }
}
