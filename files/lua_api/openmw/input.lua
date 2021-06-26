-------------------------------------------------------------------------------
-- `openmw.input` can be used only in scripts attached to a player.
-- @module input
-- @usage local input = require('openmw.input')



-------------------------------------------------------------------------------
-- Is player idle.
-- @function [parent=#input] isIdle
-- @return #boolean

-------------------------------------------------------------------------------
-- Is a specific control currently pressed.
-- Input bindings can be changed ingame using Options/Controls menu.
-- @function [parent=#input] isActionPressed
-- @param #number actionId One of @{openmw.input#ACTION}
-- @return #boolean

-------------------------------------------------------------------------------
-- Is a mouse button currently pressed.
-- @function [parent=#input] isMouseButtonPressed
-- @param #number buttonId Button index (1 - left, 2 - middle, 3 - right, 4 - X1, 5 - X2)
-- @return #boolean

-------------------------------------------------------------------------------
-- Horizontal mouse movement during the last frame.
-- @function [parent=#input] getMouseMoveX
-- @return #number

-------------------------------------------------------------------------------
-- Vertical mouse movement during the last frame.
-- @function [parent=#input] getMouseMoveY
-- @return #number

-------------------------------------------------------------------------------
-- Get value of an axis of a game controller.
-- @function [parent=#input] getAxisValue
-- @param #number axisId Index of a controller axis, one of @{openmw.input#CONTROLLER_AXIS}.
-- @return #number Value in range [-1, 1].

-------------------------------------------------------------------------------
-- Get state of a control switch. I.e. is player able to move/fight/jump/etc.
-- @function [parent=#input] getControlSwitch
-- @param #string key Control type (see @{openmw.input#CONTROL_SWITCH})
-- @return #boolean

-------------------------------------------------------------------------------
-- Set state of a control switch. I.e. forbid or allow player to move/fight/jump/etc.
-- @function [parent=#input] setControlSwitch
-- @param #string key Control type (see @{openmw.input#CONTROL_SWITCH})
-- @param #boolean value

-------------------------------------------------------------------------------
-- @type CONTROL_SWITCH
-- @field [parent=#CONTROL_SWITCH] #string Controls Ability to move
-- @field [parent=#CONTROL_SWITCH] #string Fighting Ability to attack
-- @field [parent=#CONTROL_SWITCH] #string Jumping Ability to jump
-- @field [parent=#CONTROL_SWITCH] #string Looking Ability to change view direction
-- @field [parent=#CONTROL_SWITCH] #string Magic Ability to use magic
-- @field [parent=#CONTROL_SWITCH] #string ViewMode Ability to toggle 1st/3rd person view
-- @field [parent=#CONTROL_SWITCH] #string VanityMode Vanity view if player doesn't touch controls for a long time

-------------------------------------------------------------------------------
-- Values that can be used with getControlSwitch/setControlSwitch.
-- @field [parent=#input] #CONTROL_SWITCH CONTROL_SWITCH

-------------------------------------------------------------------------------
-- @type ACTION
-- @field [parent=#ACTION] #number GameMenu
-- @field [parent=#ACTION] #number Screenshot
-- @field [parent=#ACTION] #number Inventory
-- @field [parent=#ACTION] #number Console
-- @field [parent=#ACTION] #number MoveLeft
-- @field [parent=#ACTION] #number MoveRight
-- @field [parent=#ACTION] #number MoveForward
-- @field [parent=#ACTION] #number MoveBackward
-- @field [parent=#ACTION] #number Activate
-- @field [parent=#ACTION] #number Use
-- @field [parent=#ACTION] #number Jump
-- @field [parent=#ACTION] #number AutoMove
-- @field [parent=#ACTION] #number Journal
-- @field [parent=#ACTION] #number Weapon
-- @field [parent=#ACTION] #number Spell
-- @field [parent=#ACTION] #number Run
-- @field [parent=#ACTION] #number CycleSpellLeft
-- @field [parent=#ACTION] #number CycleSpellRight
-- @field [parent=#ACTION] #number CycleWeaponLeft
-- @field [parent=#ACTION] #number CycleWeaponRight
-- @field [parent=#ACTION] #number ToggleSneak
-- @field [parent=#ACTION] #number AlwaysRun
-- @field [parent=#ACTION] #number Sneak
-- @field [parent=#ACTION] #number QuickSave
-- @field [parent=#ACTION] #number QuickLoad
-- @field [parent=#ACTION] #number QuickMenu
-- @field [parent=#ACTION] #number ToggleWeapon
-- @field [parent=#ACTION] #number ToggleSpell
-- @field [parent=#ACTION] #number TogglePOV
-- @field [parent=#ACTION] #number QuickKey1
-- @field [parent=#ACTION] #number QuickKey2
-- @field [parent=#ACTION] #number QuickKey3
-- @field [parent=#ACTION] #number QuickKey4
-- @field [parent=#ACTION] #number QuickKey5
-- @field [parent=#ACTION] #number QuickKey6
-- @field [parent=#ACTION] #number QuickKey7
-- @field [parent=#ACTION] #number QuickKey8
-- @field [parent=#ACTION] #number QuickKey9
-- @field [parent=#ACTION] #number QuickKey10
-- @field [parent=#ACTION] #number QuickKeysMenu
-- @field [parent=#ACTION] #number ToggleHUD
-- @field [parent=#ACTION] #number ToggleDebug
-- @field [parent=#ACTION] #number ZoomIn
-- @field [parent=#ACTION] #number ZoomOut

-------------------------------------------------------------------------------
-- Values that can be used with isActionPressed.
-- @field [parent=#input] #ACTION ACTION

-------------------------------------------------------------------------------
-- @type CONTROLLER_BUTTON
-- @field [parent=#CONTROLLER_BUTTON] #number A
-- @field [parent=#CONTROLLER_BUTTON] #number B
-- @field [parent=#CONTROLLER_BUTTON] #number X
-- @field [parent=#CONTROLLER_BUTTON] #number Y
-- @field [parent=#CONTROLLER_BUTTON] #number Back
-- @field [parent=#CONTROLLER_BUTTON] #number Guide
-- @field [parent=#CONTROLLER_BUTTON] #number Start
-- @field [parent=#CONTROLLER_BUTTON] #number LeftStick
-- @field [parent=#CONTROLLER_BUTTON] #number RightStick
-- @field [parent=#CONTROLLER_BUTTON] #number LeftShoulder
-- @field [parent=#CONTROLLER_BUTTON] #number RightShoulder
-- @field [parent=#CONTROLLER_BUTTON] #number DPadUp
-- @field [parent=#CONTROLLER_BUTTON] #number DPadDown
-- @field [parent=#CONTROLLER_BUTTON] #number DPadLeft
-- @field [parent=#CONTROLLER_BUTTON] #number DPadRight

-------------------------------------------------------------------------------
-- Values that can be passed to onControllerButtonPress/onControllerButtonRelease engine handlers.
-- @field [parent=#input] #CONTROLLER_BUTTON CONTROLLER_BUTTON

-------------------------------------------------------------------------------
-- Ids of game controller axises. Used as an argument in getAxisValue.
-- @type CONTROLLER_AXIS
-- @field [parent=#CONTROLLER_AXIS] #number LeftX Left stick horizontal axis (from -1 to 1)
-- @field [parent=#CONTROLLER_AXIS] #number LeftY Left stick vertical axis (from -1 to 1)
-- @field [parent=#CONTROLLER_AXIS] #number RightX Right stick horizontal axis (from -1 to 1)
-- @field [parent=#CONTROLLER_AXIS] #number RightY Right stick vertical axis (from -1 to 1)
-- @field [parent=#CONTROLLER_AXIS] #number TriggerLeft Left trigger (from 0 to 1)
-- @field [parent=#CONTROLLER_AXIS] #number TriggerRight Right trigger (from 0 to 1)
-- @field [parent=#CONTROLLER_AXIS] #number LookUpDown View direction vertical axis (RightY by default, can be mapped to another axis in Options/Controls menu)
-- @field [parent=#CONTROLLER_AXIS] #number LookLeftRight View direction horizontal axis (RightX by default, can be mapped to another axis in Options/Controls menu)
-- @field [parent=#CONTROLLER_AXIS] #number MoveForwardBackward Movement forward/backward (LeftY by default, can be mapped to another axis in Options/Controls menu)
-- @field [parent=#CONTROLLER_AXIS] #number MoveLeftRight Side movement (LeftX by default, can be mapped to another axis in Options/Controls menu)

-------------------------------------------------------------------------------
-- Values that can be used with getAxisValue.
-- @field [parent=#input] #CONTROLLER_AXIS CONTROLLER_AXIS

-------------------------------------------------------------------------------
-- The argument of `onKeyPress`/`onKeyRelease` engine handlers.
-- @type KeyboardEvent
-- @field [parent=#KeyboardEvent] #string symbol The pressed symbol (1-symbol string).
-- @field [parent=#KeyboardEvent] #string code Key code.
-- @field [parent=#KeyboardEvent] #boolean withShift Is `Shift` key pressed.
-- @field [parent=#KeyboardEvent] #boolean withCtrl Is `Control` key pressed.
-- @field [parent=#KeyboardEvent] #boolean withAlt Is `Alt` key pressed.
-- @field [parent=#KeyboardEvent] #boolean withSuper Is `Super`/`Win` key pressed.

return nil

