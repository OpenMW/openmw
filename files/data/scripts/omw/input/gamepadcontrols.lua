local input = require('openmw.input')

return {

    interfaceName = 'GamepadControls',
    ---
    -- Gamepad control interface
    -- @module GamepadControls

    interface = {
        --- Interface version
        -- @field [parent=#GamepadControls] #number version
        version = 0,

        --- Checks if the gamepad cursor is active. If it is active, the left stick can move the cursor, and A will be interpreted as a mouse click.
        -- @function [parent=#GamepadControls] isGamepadCursorActive
        -- @return #boolean
        isGamepadCursorActive = function()
            return input._isGamepadCursorActive()
        end,

        --- Set if the gamepad cursor is active. If it is active, the left stick can move the cursor, and A will be interpreted as a mouse click.
        -- @function [parent=#GamepadControls] setGamepadCursorActive
        -- @param #boolean value
        setGamepadCursorActive = function(state)
            input._setGamepadCursorActive(state)
        end,

        --- Sends an event to the interface to select the next menu element
        -- @function [parent=#GamepadControls] selectNextMenuElement
        selectNextMenuElement = function(state)
            input._selectNextMenuElement()
        end,

        --- Sends an event to the interface to select the next menu element
        -- @function [parent=#GamepadControls] selectPrevMenuElement
        selectPrevMenuElement = function(state)
            input._selectPrevMenuElement()
        end,
    }
}
