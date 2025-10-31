---
-- Most mods should prefer to use the actions/triggers API over the direct input device methods.
-- Actions have one value on each frame (resolved just before the `onFrame` engine handler),
--  while Triggers don't have a value, but can occur multiple times on each frame.
-- Prefer to use built-in methods of binding actions, such as the [inputBinding setting renderer](setting_renderers.html#inputbinding)
-- @context menu|player
-- @module input
-- @usage local input = require('openmw.input')
-- -- Example of Action usage
-- input.registerAction {
--     key = 'MyAction',
--     type = input.ACTION_TYPE.Boolean,
--     l10n = 'MyLocalizationContext',
--     name = 'MyAction_name',
--     description = 'MyAction_full_description',
--     defaultValue = false,
-- }
-- return {
--     onFrame = function()
--         local myAction = input.getBooleanActionValue('MyAction')
--         if (myAction) then print('My action is active!') end
--     end,
-- }
-- -- Example of Trigger usage
-- input.registerTrigger {
--     key = 'MyTrigger',
--     l10n = 'MyLocalizationContext',
--     name = 'MyTrigger_name',
--     description = 'MyTrigger_full_description',
-- }
-- input.registerTriggerHandler('MyTrigger', async:callback(function() print('MyTrigger') end))



---
-- Is player idle.
-- @function [parent=#input] isIdle
-- @return #boolean

---
-- (DEPRECATED, use getBooleanActionValue) Input bindings can be changed ingame using Options/Controls menu.
-- @function [parent=#input] isActionPressed
-- @param #number actionId One of @{openmw.input#ACTION}
-- @return #boolean

---
-- Is a keyboard button currently pressed.
-- @function [parent=#input] isKeyPressed
-- @param #KeyCode keyCode Key code (see @{openmw.input#KEY})
-- @return #boolean

---
-- Is a controller button currently pressed.
-- @function [parent=#input] isControllerButtonPressed
-- @param #number buttonId Button index (see @{openmw.input#CONTROLLER_BUTTON})
-- @return #boolean

---
-- Is `Shift` key pressed.
-- @function [parent=#input] isShiftPressed
-- @return #boolean

---
-- Is `Ctrl` key pressed.
-- @function [parent=#input] isCtrlPressed
-- @return #boolean

---
-- Is `Alt` key pressed.
-- @function [parent=#input] isAltPressed
-- @return #boolean

---
-- Is `Super`/`Win` key pressed.
-- @function [parent=#input] isSuperPressed
-- @return #boolean

---
-- Is a mouse button currently pressed.
-- @function [parent=#input] isMouseButtonPressed
-- @param #number buttonId Button index (1 - left, 2 - middle, 3 - right, 4 - X1, 5 - X2)
-- @return #boolean

---
-- Horizontal mouse movement during the last frame.
-- @function [parent=#input] getMouseMoveX
-- @return #number

---
-- Vertical mouse movement during the last frame.
-- @function [parent=#input] getMouseMoveY
-- @return #number

---
-- Get value of an axis of a game controller.
-- @function [parent=#input] getAxisValue
-- @param #number axisId Index of a controller axis, one of @{openmw.input#CONTROLLER_AXIS}.
-- @return #number Value in range [-1, 1].

---
-- Check if the current controller supports rumble and it is enabled in settings.
-- @function [parent=#input] controllerHasRumble
-- @return #boolean

---
-- Start a rumble effect on the active controller.
-- @function [parent=#input] controllerStartRumble
-- @param options Table of options.
-- @param options.duration #number Duration in seconds (> 0).
-- @param[opt] options.strength #number Strength applied to both motors (0.0-1.0). Can be overridden by ``options.low`` or ``options.high``.
-- @param[opt] options.low #number Low-frequency motor strength (0.0-1.0). Defaults to ``options.strength``.
-- @param[opt] options.high #number High-frequency motor strength (0.0-1.0). Defaults to ``options.low``.
-- @return #boolean ``true`` if rumble started, ``false`` if the controller can't rumble.

---
-- Stop any active controller rumble effect.
-- @function [parent=#input] controllerStopRumble

---
-- Returns a human readable name for the given key code
-- @function [parent=#input] getKeyName
-- @param #KeyCode code A key code (see @{openmw.input#KEY})
-- @return #string

---
-- [Deprecated, moved to types.Player] Get state of a control switch. I.e. is player able to move/fight/jump/etc.
-- @function [parent=#input] getControlSwitch
-- @param #ControlSwitch key Control type (see @{openmw.input#CONTROL_SWITCH})
-- @return #boolean

---
-- [Deprecated, moved to types.Player] Set state of a control switch. I.e. forbid or allow player to move/fight/jump/etc.
-- @function [parent=#input] setControlSwitch
-- @param #ControlSwitch key Control type (see @{openmw.input#CONTROL_SWITCH})
-- @param #boolean value

---
-- String id of a @{#CONTROL_SWITCH}
-- @type ControlSwitch

---
-- @type CONTROL_SWITCH
-- @field [parent=#CONTROL_SWITCH] #ControlSwitch Controls Ability to move
-- @field [parent=#CONTROL_SWITCH] #ControlSwitch Fighting Ability to attack
-- @field [parent=#CONTROL_SWITCH] #ControlSwitch Jumping Ability to jump
-- @field [parent=#CONTROL_SWITCH] #ControlSwitch Looking Ability to change view direction
-- @field [parent=#CONTROL_SWITCH] #ControlSwitch Magic Ability to use magic
-- @field [parent=#CONTROL_SWITCH] #ControlSwitch ViewMode Ability to toggle 1st/3rd person view
-- @field [parent=#CONTROL_SWITCH] #ControlSwitch VanityMode Vanity view if player doesn't touch controls for a long time

---
-- [Deprecated, moved to types.Player] Values that can be used with getControlSwitch/setControlSwitch.
-- @field [parent=#input] #CONTROL_SWITCH CONTROL_SWITCH

---
-- (DEPRECATED, use actions with matching keys)
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
-- @field [parent=#ACTION] #number Run
-- @field [parent=#ACTION] #number CycleSpellLeft
-- @field [parent=#ACTION] #number CycleSpellRight
-- @field [parent=#ACTION] #number CycleWeaponLeft
-- @field [parent=#ACTION] #number CycleWeaponRight
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
-- @field [parent=#ACTION] #number TogglePostProcessorHUD

---
-- (DEPRECATED, use getBooleanActionValue) Values that can be used with isActionPressed.
-- @field [parent=#input] #ACTION ACTION

---
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

---
-- Values that can be passed to onControllerButtonPress/onControllerButtonRelease engine handlers.
-- @field [parent=#input] #CONTROLLER_BUTTON CONTROLLER_BUTTON

---
-- Ids of game controller axises. Used as an argument in getAxisValue.
-- @type CONTROLLER_AXIS
-- @field [parent=#CONTROLLER_AXIS] #number LeftX Left stick horizontal axis (from -1 to 1)
-- @field [parent=#CONTROLLER_AXIS] #number LeftY Left stick vertical axis (from -1 to 1)
-- @field [parent=#CONTROLLER_AXIS] #number RightX Right stick horizontal axis (from -1 to 1)
-- @field [parent=#CONTROLLER_AXIS] #number RightY Right stick vertical axis (from -1 to 1)
-- @field [parent=#CONTROLLER_AXIS] #number TriggerLeft Left trigger (from 0 to 1)
-- @field [parent=#CONTROLLER_AXIS] #number TriggerRight Right trigger (from 0 to 1)
-- @field [parent=#CONTROLLER_AXIS] #number LookUpDown (DEPRECATED, use the LookUpDown action) View direction vertical axis (RightY by default, can be mapped to another axis in Options/Controls menu)
-- @field [parent=#CONTROLLER_AXIS] #number LookLeftRight (DEPRECATED, use the LookLeftRight action) View direction horizontal axis (RightX by default, can be mapped to another axis in Options/Controls menu)
-- @field [parent=#CONTROLLER_AXIS] #number MoveForwardBackward (DEPRECATED, use the MoveForwardBackward action) Movement forward/backward (LeftY by default, can be mapped to another axis in Options/Controls menu)
-- @field [parent=#CONTROLLER_AXIS] #number MoveLeftRight (DEPRECATED, use the MoveLeftRight action) Side movement (LeftX by default, can be mapped to another axis in Options/Controls menu)

---
-- Values that can be used with getAxisValue.
-- @field [parent=#input] #CONTROLLER_AXIS CONTROLLER_AXIS

---
-- Numeric id of a @{#KEY}
-- @type KeyCode

---
-- @type KEY
-- @field #KeyCode _0
-- @field #KeyCode _1
-- @field #KeyCode _2
-- @field #KeyCode _3
-- @field #KeyCode _4
-- @field #KeyCode _5
-- @field #KeyCode _6
-- @field #KeyCode _7
-- @field #KeyCode _8
-- @field #KeyCode _9
-- @field #KeyCode NP_0
-- @field #KeyCode NP_1
-- @field #KeyCode NP_2
-- @field #KeyCode NP_3
-- @field #KeyCode NP_4
-- @field #KeyCode NP_5
-- @field #KeyCode NP_6
-- @field #KeyCode NP_7
-- @field #KeyCode NP_8
-- @field #KeyCode NP_9
-- @field #KeyCode NP_Divide
-- @field #KeyCode NP_Enter
-- @field #KeyCode NP_Minus
-- @field #KeyCode NP_Multiply
-- @field #KeyCode NP_Delete
-- @field #KeyCode NP_Plus
-- @field #KeyCode F1
-- @field #KeyCode F2
-- @field #KeyCode F3
-- @field #KeyCode F4
-- @field #KeyCode F5
-- @field #KeyCode F6
-- @field #KeyCode F7
-- @field #KeyCode F8
-- @field #KeyCode F9
-- @field #KeyCode F10
-- @field #KeyCode F11
-- @field #KeyCode F12
-- @field #KeyCode A
-- @field #KeyCode B
-- @field #KeyCode C
-- @field #KeyCode D
-- @field #KeyCode E
-- @field #KeyCode F
-- @field #KeyCode G
-- @field #KeyCode H
-- @field #KeyCode I
-- @field #KeyCode J
-- @field #KeyCode K
-- @field #KeyCode L
-- @field #KeyCode M
-- @field #KeyCode N
-- @field #KeyCode O
-- @field #KeyCode P
-- @field #KeyCode Q
-- @field #KeyCode R
-- @field #KeyCode S
-- @field #KeyCode T
-- @field #KeyCode U
-- @field #KeyCode V
-- @field #KeyCode W
-- @field #KeyCode X
-- @field #KeyCode Y
-- @field #KeyCode Z
-- @field #KeyCode LeftArrow
-- @field #KeyCode RightArrow
-- @field #KeyCode UpArrow
-- @field #KeyCode DownArrow
-- @field #KeyCode LeftAlt
-- @field #KeyCode LeftCtrl
-- @field #KeyCode LeftBracket
-- @field #KeyCode LeftSuper
-- @field #KeyCode LeftShift
-- @field #KeyCode RightAlt
-- @field #KeyCode RightCtrl
-- @field #KeyCode RightBracket
-- @field #KeyCode RightSuper
-- @field #KeyCode RightShift
-- @field #KeyCode Apostrophe
-- @field #KeyCode BackSlash
-- @field #KeyCode Backspace
-- @field #KeyCode CapsLock
-- @field #KeyCode Comma
-- @field #KeyCode Delete
-- @field #KeyCode End
-- @field #KeyCode Enter
-- @field #KeyCode Equals
-- @field #KeyCode Escape
-- @field #KeyCode Home
-- @field #KeyCode Insert
-- @field #KeyCode Minus
-- @field #KeyCode NumLock
-- @field #KeyCode PageDown
-- @field #KeyCode PageUp
-- @field #KeyCode Pause
-- @field #KeyCode Period
-- @field #KeyCode PrintScreen
-- @field #KeyCode ScrollLock
-- @field #KeyCode Semicolon
-- @field #KeyCode Slash
-- @field #KeyCode Space
-- @field #KeyCode Tab

---
-- Key codes.
-- @field [parent=#input] #KEY KEY

---
-- The argument of `onKeyPress`/`onKeyRelease` engine handlers.
-- @type KeyboardEvent
-- @field [parent=#KeyboardEvent] #string symbol The pressed symbol (1-symbol string if can be represented or an empty string otherwise).
-- @field [parent=#KeyboardEvent] #KeyCode code Key code.
-- @field [parent=#KeyboardEvent] #boolean withShift Is `Shift` key pressed.
-- @field [parent=#KeyboardEvent] #boolean withCtrl Is `Control` key pressed.
-- @field [parent=#KeyboardEvent] #boolean withAlt Is `Alt` key pressed.
-- @field [parent=#KeyboardEvent] #boolean withSuper Is `Super`/`Win` key pressed.

---
-- The argument of onTouchPress/onTouchRelease/onTouchMove engine handlers.
-- @type TouchEvent
-- @field [parent=#TouchEvent] #number device Device id (there might be multiple touch devices connected). Note: the specific device ids are not guaranteed. Always use previous user input (onTouch... handlers) to get a valid device id (e. g. in your script's settings page).
-- @field [parent=#TouchEvent] #number finger Finger id (the device might support multitouch).
-- @field [parent=#TouchEvent] openmw.util#Vector2 position Relative position on the touch device (0 to 1 from top left corner),
-- @field [parent=#TouchEvent] #number pressure Pressure of the finger.

---
-- @type ActionType

---
-- @type ACTION_TYPE
-- @field #ActionType Boolean Input action with value of true or false
-- @field #ActionType Number Input action with a numeric value
-- @field #ActionType Range Input action with a numeric value between 0 and 1 (inclusive)

---
-- Values that can be used in registerAction
-- @field [parent=#input] #ACTION_TYPE ACTION_TYPE

---
-- @type ActionInfo
-- @field [parent=#Actioninfo] #string key
-- @field [parent=#Actioninfo] #ActionType type
-- @field [parent=#Actioninfo] #string l10n Localization context containing the name and description keys
-- @field [parent=#Actioninfo] #string name Localization key of the action's name
-- @field [parent=#Actioninfo] #string description Localization key of the action's description
-- @field [parent=#Actioninfo] defaultValue initial value of the action

---
-- Map of all currently registered actions
-- @field [parent=#input] #map<#string,#ActionInfo> actions

---
-- Registers a new input action. The key must be unique
-- @function [parent=#input] registerAction
-- @param #ActionInfo info

---
-- Provides a function computing the value of given input action.
--   The callback is called once a frame, after the values of dependency actions are resolved.
--   Throws an error if a cyclic action dependency is detected.
-- @function [parent=#input] bindAction
-- @param #string key
-- @param openmw.async#Callback callback returning the new value of the action, and taking as arguments:
--   frame time in seconds,
--   value of the function,
--   value of the first dependency action,
--   ...
-- @param #list<#string> dependencies
-- @usage
--   input.bindAction('Activate', async:callback(function(dt, use, sneak, run)
--       -- while sneaking, only activate things while holding the run binding
--       return use and (run or not sneak)
--   end), { 'Sneak', 'Run' })

---
-- Registers a function to be called whenever the action's value changes
-- @function [parent=#input] registerActionHandler
-- @param #string key
-- @param openmw.async#Callback callback takes the new action value as the only argument

---
-- Returns the value of a Boolean action
-- @function [parent=#input] getBooleanActionValue
-- @param #string key
-- @return #boolean

---
-- Returns the value of a Number action
-- @function [parent=#input] getNumberActionValue
-- @param #string key
-- @return #number

---
-- Returns the value of a Range action
-- @function [parent=#input] getRangeActionValue
-- @param #string key
-- @return #number

---
-- @type TriggerInfo
-- @field [parent=#TriggerInfo] #string key
-- @field [parent=#TriggerInfo] #string l10n Localization context containing the name and description keys
-- @field [parent=#TriggerInfo] #string name Localization key of the trigger's name
-- @field [parent=#TriggerInfo] #string description Localization key of the trigger's description

---
-- Map of all currently registered triggers
-- @field [parent=#input] #map<#string,#TriggerInfo> triggers

---
-- Registers a new input trigger. The key must be unique
-- @function [parent=#input] registerTrigger
-- @param #TriggerInfo info

---
-- Registers a function to be called whenever the trigger activates
-- @function [parent=#input] registerTriggerHandler
-- @param #string key
-- @param openmw.async#Callback callback takes the new action value as the only argument

---
-- Activates the trigger with the given key
-- @function [parent=#input] activateTrigger
-- @param #string key


return nil
