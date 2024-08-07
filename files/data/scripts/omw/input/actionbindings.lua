local input = require('openmw.input')
local util = require('openmw.util')
local async = require('openmw.async')
local storage = require('openmw.storage')

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

do -- Actions and Triggers currently unused by builtin scripts
    -- TODO: as more mechanics are dehardcoded, move these declarations to relevant files
    local triggers = {
        Activate = input.ACTION.Activate,
        Console = input.ACTION.Console,
        CycleSpellLeft = input.ACTION.CycleSpellLeft,
        CycleSpellRight = input.ACTION.CycleSpellRight,
        CycleWeaponLeft = input.ACTION.CycleWeaponLeft,
        CycleWeaponRight = input.ACTION.CycleWeaponRight,
        GameMenu = input.ACTION.GameMenu,
        QuickLoad = input.ACTION.QuickLoad,
        QuickSave = input.ACTION.QuickSave,
        Screenshot = input.ACTION.Screenshot,
        ToggleDebug = input.ACTION.ToggleDebug,
        ToggleHUD = input.ACTION.ToggleHUD,
        TogglePostProcessorHUD = input.ACTION.ToggleHUD,
    }
    for i = 1, 9 do
        local key = string.format('QuickKey%s', i)
        triggers[key] = input.ACTION[key]
    end
    for  key, action in pairs(triggers) do
        input.registerTrigger {
            key = key,
            l10n = 'OMWControls',
            name = key .. '_name',
            description = key .. '_description',
        }
        bindTrigger(key, action)
    end
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

local devices = {
    keyboard = true,
    mouse = true,
    controller = true
}

local function invalidBinding(binding)
    if not binding.key then
        return 'has no key'
    elseif binding.type ~= 'action' and binding.type ~= 'trigger' then
        return string.format('has invalid type', binding.type)
    elseif binding.type == 'action' and not input.actions[binding.key] then
        return string.format("action %s doesn't exist", binding.key)
    elseif binding.type == 'trigger' and not input.triggers[binding.key] then
        return string.format("trigger %s doesn't exist", binding.key)
    elseif not binding.device or not devices[binding.device] then
        return string.format("invalid device %s", binding.device)
    elseif not binding.button then
        return 'has no button'
    end
end

local boundActions = {}
local actionBindings = {}

local function bindAction(binding, id)
    local action = binding.key
    actionBindings[action] = actionBindings[action] or {}
    actionBindings[action][id] = binding
    if not boundActions[action] then
        boundActions[binding.key] = true
        input.bindAction(action, async:callback(function()
            for _, binding in pairs(actionBindings[action] or {}) do
                if binding.device == 'keyboard' then
                    if input.isKeyPressed(binding.button) then
                        return true
                    end
                elseif binding.device == 'mouse' then
                    if input.isMouseButtonPressed(binding.button) then
                        return true
                    end
                elseif binding.device == 'controller' then
                    if input.isControllerButtonPressed(binding.button) then
                        return true
                    end
                end
            end
            return false
        end), {})
    end
end

local triggerBindings = {}
for device in pairs(devices) do triggerBindings[device] = {} end

local function bindTrigger(binding, id)
    local deviceBindings = triggerBindings[binding.device]
    deviceBindings[binding.button] = deviceBindings[binding.button] or {}
    deviceBindings[binding.button][id] = binding
end

local function registerBinding(binding, id)
    local invalid = invalidBinding(binding)
    if invalid then
        print(string.format('Skipping invalid binding %s: %s', id, invalid))
    elseif binding.type == 'action' then
        bindAction(binding, id)
    elseif binding.type == 'trigger' then
        bindTrigger(binding, id)
    end
end

function clearBinding(id)
    for _, deviceBindings in pairs(triggerBindings) do
        for _, buttonBindings in pairs(deviceBindings) do
            buttonBindings[id] = nil
        end
    end

    for _, bindings in pairs(actionBindings) do
        bindings[id] = nil
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
            local buttonTriggers = triggerBindings.keyboard[e.code]
            if not buttonTriggers then return end
            for _, binding in pairs(buttonTriggers) do
                input.activateTrigger(binding.key)
            end
        end,
        onMouseButtonPress = function(button)
            local buttonTriggers = triggerBindings.mouse[button]
            if not buttonTriggers then return end
            for _, binding in pairs(buttonTriggers) do
                input.activateTrigger(binding.key)
            end
        end,
        onControllerButtonPress = function(id)
            local buttonTriggers = triggerBindings.controller[id]
            if not buttonTriggers then return end
            for _, binding in pairs(buttonTriggers) do
                input.activateTrigger(binding.key)
            end
        end,
    }
}
