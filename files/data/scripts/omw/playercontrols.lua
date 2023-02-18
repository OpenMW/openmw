local core = require('openmw.core')
local input = require('openmw.input')
local self = require('openmw.self')
local util = require('openmw.util')
local ui = require('openmw.ui')
local Actor = require('openmw.types').Actor
local Player = require('openmw.types').Player

local storage = require('openmw.storage')
local I = require('openmw.interfaces')

local settingsGroup = 'SettingsOMWControls'

local function boolSetting(key, default)
    return {
        key = key,
        renderer = 'checkbox',
        name = key,
        description = key..'Description',
        default = default,
    }
end

I.Settings.registerPage({
  key = 'OMWControls',
  l10n = 'OMWControls',
  name = 'ControlsPage',
  description = 'ControlsPageDescription',
})

I.Settings.registerGroup({
    key = settingsGroup,
    page = 'OMWControls',
    l10n = 'OMWControls',
    name = 'MovementSettings',
    permanentStorage = true,
    settings = {
        boolSetting('alwaysRun', false),
        boolSetting('toggleSneak', false),
    },
})

local settings = storage.playerSection(settingsGroup)

local attemptJump = false
local startAttack = false
local autoMove = false
local movementControlsOverridden = false
local combatControlsOverridden = false

local function processMovement()
    local controllerMovement = -input.getAxisValue(input.CONTROLLER_AXIS.MoveForwardBackward)
    local controllerSideMovement = input.getAxisValue(input.CONTROLLER_AXIS.MoveLeftRight)
    if controllerMovement ~= 0 or controllerSideMovement ~= 0 then
        -- controller movement
        if util.vector2(controllerMovement, controllerSideMovement):length2() < 0.25
           and not self.controls.sneak and Actor.isOnGround(self) and not Actor.isSwimming(self) then
            self.controls.run = false
            self.controls.movement = controllerMovement * 2
            self.controls.sideMovement = controllerSideMovement * 2
        else
            self.controls.run = true
            self.controls.movement = controllerMovement
            self.controls.sideMovement = controllerSideMovement
        end
    else
        -- keyboard movement
        self.controls.movement = 0
        self.controls.sideMovement = 0
        if input.isActionPressed(input.ACTION.MoveLeft) then
            self.controls.sideMovement = self.controls.sideMovement - 1
        end
        if input.isActionPressed(input.ACTION.MoveRight) then
            self.controls.sideMovement = self.controls.sideMovement + 1
        end
        if input.isActionPressed(input.ACTION.MoveBackward) then
            self.controls.movement = self.controls.movement - 1
        end
        if input.isActionPressed(input.ACTION.MoveForward) then
            self.controls.movement = self.controls.movement + 1
        end
        self.controls.run = input.isActionPressed(input.ACTION.Run) ~= settings:get('alwaysRun')
    end
    if self.controls.movement ~= 0 or not Actor.canMove(self) then
        autoMove = false
    elseif autoMove then
        self.controls.movement = 1
    end
    self.controls.jump = attemptJump and input.getControlSwitch(input.CONTROL_SWITCH.Jumping)
    if not settings:get('toggleSneak') then
        self.controls.sneak = input.isActionPressed(input.ACTION.Sneak)
    end
end

local function processAttacking()
    if startAttack then
        self.controls.use = 1
    elseif Actor.stance(self) == Actor.STANCE.Spell then
        self.controls.use = 0
    elseif input.getAxisValue(input.CONTROLLER_AXIS.TriggerRight) < 0.6
           and not input.isActionPressed(input.ACTION.Use) then
        -- The value "0.6" shouldn't exceed the triggering threshold in BindingsManager::actionValueChanged.
        -- TODO: Move more logic from BindingsManager to Lua and consider to make this threshold configurable.
        self.controls.use = 0
    end
end

local function onFrame(dt)
    controlsAllowed = input.getControlSwitch(input.CONTROL_SWITCH.Controls) and not core.isWorldPaused()
    if not movementControlsOverridden then
        if controlsAllowed then
            processMovement()
        else
            self.controls.movement = 0
            self.controls.sideMovement = 0
            self.controls.jump = false
        end
    end
    if controlsAllowed and not combatControlsOverridden then
        processAttacking()
    end
    attemptJump = false
    startAttack = false
end

local function onInputAction(action)
    if core.isWorldPaused() or not input.getControlSwitch(input.CONTROL_SWITCH.Controls) then
        return
    end

    if action == input.ACTION.Jump then
        attemptJump = true
    elseif action == input.ACTION.Use then
        startAttack = Actor.stance(self) ~= Actor.STANCE.Nothing
    elseif action == input.ACTION.AutoMove and not movementControlsOverridden then
        autoMove = not autoMove
    elseif action == input.ACTION.AlwaysRun and not movementControlsOverridden then
        settings:set('alwaysRun', not settings:get('alwaysRun'))
    elseif action == input.ACTION.Sneak and not movementControlsOverridden then
        if settings:get('toggleSneak') then
            self.controls.sneak = not self.controls.sneak
        end
    elseif action == input.ACTION.ToggleSpell and not combatControlsOverridden then
        if Actor.stance(self) == Actor.STANCE.Spell then
            Actor.setStance(self, Actor.STANCE.Nothing)
        elseif input.getControlSwitch(input.CONTROL_SWITCH.Magic) then
            if Player.isWerewolf(self) then
                ui.showMessage(core.getGMST('sWerewolfRefusal'))
            else
                Actor.setStance(self, Actor.STANCE.Spell)
            end
        end
    elseif action == input.ACTION.ToggleWeapon and not combatControlsOverridden then
        if Actor.stance(self) == Actor.STANCE.Weapon then
            Actor.setStance(self, Actor.STANCE.Nothing)
        elseif input.getControlSwitch(input.CONTROL_SWITCH.Fighting) then
            Actor.setStance(self, Actor.STANCE.Weapon)
        end
    end
end

return {
    engineHandlers = {
        onFrame = onFrame,
        onInputAction = onInputAction,
    },
    interfaceName = 'Controls',
    ---
    -- @module Controls
    -- @usage require('openmw.interfaces').Controls
    interface = {
        --- Interface version
        -- @field [parent=#Controls] #number version
        version = 0,

        --- When set to true then the movement controls including jump and sneak are not processed and can be handled by another script.
        -- If movement should be dissallowed completely, consider to use `input.setControlSwitch` instead.
        -- @function [parent=#Controls] overrideMovementControls
        -- @param #boolean value
        overrideMovementControls = function(v) movementControlsOverridden = v end,

        --- When set to true then the controls "attack", "toggle spell", "toggle weapon" are not processed and can be handled by another script.
        -- If combat should be dissallowed completely, consider to use `input.setControlSwitch` instead.
        -- @function [parent=#Controls] overrideCombatControls
        -- @param #boolean value
        overrideCombatControls = function(v) combatControlsOverridden = v end,
    }
}

