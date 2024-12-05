local core = require('openmw.core')
local input = require('openmw.input')
local self = require('openmw.self')
local storage = require('openmw.storage')
local ui = require('openmw.ui')
local async = require('openmw.async')
local Actor = require('openmw.types').Actor
local Player = require('openmw.types').Player

local I = require('openmw.interfaces')

local settings = storage.playerSection('SettingsOMWControls')

do
    local rangeActions = {
        'MoveForward',
        'MoveBackward',
        'MoveLeft',
        'MoveRight'
    }
    for _, key in ipairs(rangeActions) do
        input.registerAction {
            key = key,
            l10n = 'OMWControls',
            name = key .. '_name',
            description = key .. '_description',
            type = input.ACTION_TYPE.Range,
            defaultValue = 0,
        }
    end

    local booleanActions = {
        'Use',
        'Run',
        'Sneak',
    }
    for _, key in ipairs(booleanActions) do
        input.registerAction {
            key = key,
            l10n = 'OMWControls',
            name = key .. '_name',
            description = key .. '_description',
            type = input.ACTION_TYPE.Boolean,
            defaultValue = false,
        }
    end

    local triggers = {
        'Jump',
        'AutoMove',
        'ToggleWeapon',
        'ToggleSpell',
        'AlwaysRun',
        'ToggleSneak',
        'Inventory',
        'Journal',
        'QuickKeysMenu',
    }
    for _, key in ipairs(triggers) do
        input.registerTrigger {
            key = key,
            l10n = 'OMWControls',
            name = key .. '_name',
            description = key .. '_description',
        }
    end
end

local function checkNotWerewolf()
    if Player.isWerewolf(self) then
        ui.showMessage(core.getGMST('sWerewolfRefusal'))
        return false
    else
        return true
    end
end

local function isJournalAllowed()
    -- During chargen journal is not allowed until magic window is allowed
    return I.UI.getWindowsForMode(I.UI.MODE.Interface)[I.UI.WINDOW.Magic]
end

local movementControlsOverridden = false

local autoMove = false
local attemptToJump = false
local function processMovement()
    local movement = input.getRangeActionValue('MoveForward') - input.getRangeActionValue('MoveBackward')
    local sideMovement = input.getRangeActionValue('MoveRight') - input.getRangeActionValue('MoveLeft')
    local run = input.getBooleanActionValue('Run') ~= settings:get('alwaysRun')

    if movement ~= 0 or not Actor.canMove(self) then
        autoMove = false
    elseif autoMove then
        movement = 1
    end

    self.controls.movement = movement
    self.controls.sideMovement = sideMovement
    self.controls.run = run
    self.controls.jump = attemptToJump

    if not settings:get('toggleSneak') then
        self.controls.sneak = input.getBooleanActionValue('Sneak')
    end
end

local function controlsAllowed()
    return not core.isWorldPaused()
        and Player.getControlSwitch(self, Player.CONTROL_SWITCH.Controls)
        and not I.UI.getMode()
end

local function movementAllowed()
    return controlsAllowed() and not movementControlsOverridden
end

input.registerTriggerHandler('Jump', async:callback(function()
    if not movementAllowed() then return end
    attemptToJump = Player.getControlSwitch(self, Player.CONTROL_SWITCH.Jumping)
end))

input.registerTriggerHandler('ToggleSneak', async:callback(function()
    if not movementAllowed() then return end
    if settings:get('toggleSneak') then
        self.controls.sneak = not self.controls.sneak
    end
end))

input.registerTriggerHandler('AlwaysRun', async:callback(function()
    if not movementAllowed() then return end
    settings:set('alwaysRun', not settings:get('alwaysRun'))
end))

input.registerTriggerHandler('AutoMove', async:callback(function()
    if not movementAllowed() then return end
    autoMove = not autoMove
end))

local combatControlsOverridden = false

local function combatAllowed()
    return controlsAllowed() and not combatControlsOverridden
end

input.registerTriggerHandler('ToggleSpell', async:callback(function()
    if not combatAllowed() then return end
    if Actor.getStance(self) == Actor.STANCE.Spell then
        Actor.setStance(self, Actor.STANCE.Nothing)
    elseif Player.getControlSwitch(self, Player.CONTROL_SWITCH.Magic) then
        if checkNotWerewolf() then
            Actor.setStance(self, Actor.STANCE.Spell)
        end
    end
end))

input.registerTriggerHandler('ToggleWeapon', async:callback(function()
    if not combatAllowed() then return end
    if Actor.getStance(self) == Actor.STANCE.Weapon then
        Actor.setStance(self, Actor.STANCE.Nothing)
    elseif Player.getControlSwitch(self, Player.CONTROL_SWITCH.Fighting) then
        Actor.setStance(self, Actor.STANCE.Weapon)
    end
end))

local startUse = false
input.registerActionHandler('Use', async:callback(function(value)
    if value and combatAllowed() then startUse = true end
end))
local function processAttacking()
    -- for spell-casting, set controls.use to true for exactly one frame
    -- otherwise spell casting is attempted every frame while Use is true
    if Actor.getStance(self) == Actor.STANCE.Spell then
        self.controls.use = startUse and 1 or 0
    elseif Actor.getStance(self) == Actor.STANCE.Weapon and input.getBooleanActionValue('Use') then
        self.controls.use = 1
    else
        self.controls.use = 0
    end
    startUse = false
end

local uiControlsOverridden = false

local function uiAllowed()
    return Player.getControlSwitch(self, Player.CONTROL_SWITCH.Controls) and not uiControlsOverridden
end

input.registerTriggerHandler('Inventory', async:callback(function()
    if not uiAllowed() then return end

    if I.UI.getMode() == nil then
        I.UI.setMode(I.UI.MODE.Interface)
    elseif I.UI.getMode() == I.UI.MODE.Interface or I.UI.getMode() == I.UI.MODE.Container then
        I.UI.removeMode(I.UI.getMode())
    end
end))

input.registerTriggerHandler('Journal', async:callback(function()
    if not uiAllowed() then return end

    if I.UI.getMode() == I.UI.MODE.Journal then
        I.UI.removeMode(I.UI.MODE.Journal)
    elseif isJournalAllowed() then
        I.UI.addMode(I.UI.MODE.Journal)
    end
end))

input.registerTriggerHandler('QuickKeysMenu', async:callback(function()
    if not uiAllowed() then return end

    if I.UI.getMode() == I.UI.MODE.QuickKeysMenu then
        I.UI.removeMode(I.UI.MODE.QuickKeysMenu)
    elseif checkNotWerewolf() and Player.isCharGenFinished(self) then
        I.UI.addMode(I.UI.MODE.QuickKeysMenu)
    end
end))

local function onFrame(_)
    if movementAllowed() then
        processMovement()
    elseif not movementControlsOverridden then
        self.controls.movement = 0
        self.controls.sideMovement = 0
        self.controls.jump = false
    end
    if combatAllowed() then
        processAttacking()
    end
    attemptToJump = false
end

local function onSave()
    return {
        sneaking = self.controls.sneak
    }
end

local function onLoad(data)
    if not data then return end
    self.controls.sneak = data.sneaking or false
end

return {
    engineHandlers = {
        onFrame = onFrame,
        onSave = onSave,
        onLoad = onLoad,
    },
    interfaceName = 'Controls',
    ---
    -- @module Controls
    -- @usage require('openmw.interfaces').Controls
    interface = {
        --- Interface version
        -- @field [parent=#Controls] #number version
        version = 1,

        --- When set to true then the movement controls including jump and sneak are not processed and can be handled by another script.
        -- If movement should be disallowed completely, consider to use `types.Player.setControlSwitch` instead.
        -- @function [parent=#Controls] overrideMovementControls
        -- @param #boolean value
        overrideMovementControls = function(v) movementControlsOverridden = v end,

        --- When set to true then the controls "attack", "toggle spell", "toggle weapon" are not processed and can be handled by another script.
        -- If combat should be disallowed completely, consider to use `types.Player.setControlSwitch` instead.
        -- @function [parent=#Controls] overrideCombatControls
        -- @param #boolean value
        overrideCombatControls = function(v) combatControlsOverridden = v end,

        --- When set to true then the controls "open inventory", "open journal" and so on are not processed and can be handled by another script.
        -- @function [parent=#Controls] overrideUiControls
        -- @param #boolean value
        overrideUiControls = function(v) uiControlsOverridden = v end,
    }
}
