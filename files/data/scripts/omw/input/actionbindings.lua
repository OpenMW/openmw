local core = require('openmw.core')
local input = require('openmw.input')
local util = require('openmw.util')
local async = require('openmw.async')
local storage = require('openmw.storage')
local ui = require('openmw.ui')

local I = require('openmw.interfaces')

local actionPressHandlers = {}
local function onActionPress(id, handler)
    actionPressHandlers[id] = actionPressHandlers[id] or {}
    table.insert(actionPressHandlers[id], handler)
end

local function bindHold(key, actionId)
    input.bindAction(key, async:callback(function()
        return input.isActionPressed(actionId)
    end), {})
end

local function bindMovement(key, actionId, axisId, direction)
    input.bindAction(key, async:callback(function()
        local actionActive = input.isActionPressed(actionId)
        local axisActive = input.getAxisValue(axisId) * direction > 0
        return (actionActive or axisActive) and 1 or 0
    end), {})
end

local function bindTrigger(key, actionid)
    onActionPress(actionid, function()
        input.activateTrigger(key)
    end)
end

bindTrigger('AlwaysRun', input.ACTION.AlwaysRun)
bindTrigger('ToggleSneak', input.ACTION.Sneak)
bindTrigger('ToggleWeapon', input.ACTION.ToggleWeapon)
bindTrigger('ToggleSpell', input.ACTION.ToggleSpell)
bindTrigger('Jump', input.ACTION.Jump)
bindTrigger('AutoMove', input.ACTION.AutoMove)
bindTrigger('Inventory', input.ACTION.Inventory)
bindTrigger('Journal', input.ACTION.Journal)
bindTrigger('QuickKeysMenu', input.ACTION.QuickKeysMenu)

bindHold('TogglePOV', input.ACTION.TogglePOV)
bindHold('Sneak', input.ACTION.Sneak)

bindHold('Run', input.ACTION.Run)
input.bindAction('Run', async:callback(function(_, value)
    local controllerInput = util.vector2(
        input.getAxisValue(input.CONTROLLER_AXIS.MoveForwardBackward),
        input.getAxisValue(input.CONTROLLER_AXIS.MoveLeftRight)
    ):length2()
    return value or controllerInput > 0.25
end), {})

input.bindAction('Use', async:callback(function()
    -- The value "0.6" shouldn't exceed the triggering threshold in BindingsManager::actionValueChanged.
    -- TODO: Move more logic from BindingsManager to Lua and consider to make this threshold configurable.
    return input.isActionPressed(input.ACTION.Use) or input.getAxisValue(input.CONTROLLER_AXIS.TriggerRight) >= 0.6
end), {})

bindMovement('MoveBackward', input.ACTION.MoveBackward, input.CONTROLLER_AXIS.MoveForwardBackward, 1)
bindMovement('MoveForward', input.ACTION.MoveForward, input.CONTROLLER_AXIS.MoveForwardBackward, -1)
bindMovement('MoveRight', input.ACTION.MoveRight, input.CONTROLLER_AXIS.MoveLeftRight, 1)
bindMovement('MoveLeft', input.ACTION.MoveLeft, input.CONTROLLER_AXIS.MoveLeftRight, -1)

do
    local zoomInOut = 0
    onActionPress(input.ACTION.ZoomIn, function()
        zoomInOut = zoomInOut + 1
    end)
    onActionPress(input.ACTION.ZoomOut, function()
        zoomInOut = zoomInOut - 1
    end)
    input.bindAction('Zoom3rdPerson', async:callback(function(dt, _, togglePOV)
        local Zoom3rdPerson = zoomInOut * 10
        if togglePOV then
            local triggerLeft = input.getAxisValue(input.CONTROLLER_AXIS.TriggerLeft)
            local triggerRight = input.getAxisValue(input.CONTROLLER_AXIS.TriggerRight)
            local controllerZoom = (triggerRight - triggerLeft) * 100 * dt
            Zoom3rdPerson = Zoom3rdPerson + controllerZoom
        end
        zoomInOut = 0
        return Zoom3rdPerson
    end), { 'TogglePOV' })
end

local bindingSection = storage.playerSection('OMWInputBindings')

local keyboardPresses = {}
local keybordHolds = {}
local boundActions = {}

local function bindAction(action)
    if boundActions[action] then return end
    boundActions[action] = true
    input.bindAction(action, async:callback(function()
        if keybordHolds[action] then
            for _, binding in pairs(keybordHolds[action]) do
                if input.isKeyPressed(binding.code) then return true end
            end
        end
        return false
    end), {})
end

local function registerBinding(binding, id)
    if not input.actions[binding.key] and not input.triggers[binding.key] then
        print(string.format('Skipping binding for unknown action or trigger: "%s"', binding.key))
        return
    end
    if binding.type == 'keyboardPress' then
        local bindings = keyboardPresses[binding.code] or {}
        bindings[id] = binding
        keyboardPresses[binding.code] = bindings
    elseif binding.type == 'keyboardHold' then
        local bindings = keybordHolds[binding.key] or {}
        bindings[id] = binding
        keybordHolds[binding.key] = bindings
        bindAction(binding.key)
    else
        error('Unknown binding type "' .. binding.type .. '"')
    end
end

function clearBinding(id)
    for _, boundTriggers in pairs(keyboardPresses) do
        boundTriggers[id] = nil
    end
    for _, boundKeys in pairs(keybordHolds) do
        boundKeys[id] = nil
    end
end

bindingSection:subscribe(async:callback(function(_, id)
    if not id then return end
    local binding = bindingSection:get(id)
    clearBinding(id)
    if binding ~= nil then
        registerBinding(binding, id)
    end
    return id
end))


local initiated = false

local function init()
    for id, binding in pairs(bindingSection:asTable()) do
        registerBinding(binding, id)
    end
end

return {
    engineHandlers = {
        onFrame = function()
            if not initiated then
                initiated = true
                init()
            end
        end,
        onInputAction = function(id)
            if not actionPressHandlers[id] then
                return
            end
            for _, handler in ipairs(actionPressHandlers[id]) do
                handler()
            end
        end,
        onKeyPress = function(e)
            local bindings = keyboardPresses[e.code]
            if bindings then
                for _, binding in pairs(bindings) do
                    input.activateTrigger(binding.key)
                end
            end
        end,
    }
}
