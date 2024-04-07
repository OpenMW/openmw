local core = require('openmw.core')
local input = require('openmw.input')
local storage = require('openmw.storage')
local ui = require('openmw.ui')
local util = require('openmw.util')
local async = require('openmw.async')
local I = require('openmw.interfaces')

local settingsGroup = 'SettingsOMWControls'

local function boolSetting(key, default)
    return {
        key = key,
        renderer = 'checkbox',
        name = key,
        description = key .. 'Description',
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
        boolSetting('toggleSneak', false), -- TODO: consider removing this setting when we have the advanced binding UI
        boolSetting('smoothControllerMovement', true),
    },
})

local interfaceL10n = core.l10n('interface')

local bindingSection = storage.playerSection('OMWInputBindings')

local recording = nil

local mouseButtonNames = {
    [1] = 'Left',
    [2] = 'Middle',
    [3] = 'Right',
    [4] = '4',
    [5] = '5',
}

-- TODO: support different controllers, use icons to render controller buttons
local controllerButtonNames = {
    [-1] = 'Invalid',
    [input.CONTROLLER_BUTTON.A] = "A",
    [input.CONTROLLER_BUTTON.B] = "B",
    [input.CONTROLLER_BUTTON.X] = "X",
    [input.CONTROLLER_BUTTON.Y] = "Y",
    [input.CONTROLLER_BUTTON.Back] = "Back",
    [input.CONTROLLER_BUTTON.Guide] = "Guide",
    [input.CONTROLLER_BUTTON.Start] = "Start",
    [input.CONTROLLER_BUTTON.LeftStick] = "Left Stick",
    [input.CONTROLLER_BUTTON.RightStick] = "Right Stick",
    [input.CONTROLLER_BUTTON.LeftShoulder] = "LB",
    [input.CONTROLLER_BUTTON.RightShoulder] = "RB",
    [input.CONTROLLER_BUTTON.DPadUp] = "D-pad Up",
    [input.CONTROLLER_BUTTON.DPadDown] = "D-pad Down",
    [input.CONTROLLER_BUTTON.DPadLeft] = "D-pad Left",
    [input.CONTROLLER_BUTTON.DPadRight] = "D-pad Right",
}

local function bindingLabel(recording, binding)
    if recording then
        return interfaceL10n('N/A')
    elseif not binding or not binding.button then
        return interfaceL10n('None')
    elseif binding.device == 'keyboard' then
        return input.getKeyName(binding.button)
    elseif binding.device == 'mouse' then
        return string.format('Mouse %s', mouseButtonNames[binding.button] or 'Unknown')
    elseif binding.device == 'controller' then
        return string.format('Controller %s', controllerButtonNames[binding.button] or 'Unknown')
    else
        return 'Unknown'
    end
end

local inputTypes = {
    action = input.actions,
    trigger = input.triggers,
}

I.Settings.registerRenderer('inputBinding', function(id, set, arg)
    if type(id) ~= 'string' then error('inputBinding: must have a string default value') end
    if not arg then error('inputBinding: argument with "key" and "type" is required') end
    if not arg.type then error('inputBinding: type argument is required') end
    if not inputTypes[arg.type] then error('inputBinding: type must be "action" or "trigger"') end
    if not arg.key then error('inputBinding: key argument is required') end
    local info = inputTypes[arg.type][arg.key]
    if not info then print(string.format('inputBinding: %s %s not found', arg.type, arg.key)) return end

    local l10n = core.l10n(info.l10n)

    local name = {
        template = I.MWUI.templates.textNormal,
        props = {
            text = l10n(info.name),
        },
    }

    local description = {
        template = I.MWUI.templates.textNormal,
        props = {
            text = l10n(info.description),
        },
    }

    local binding = bindingSection:get(id)
    local label = bindingLabel(recording and recording.id == id, binding)

    local recorder = {
        template = I.MWUI.templates.textNormal,
        props = {
            text = label,
        },
        events = {
            mouseClick = async:callback(function()
                if recording ~= nil then return end
                if binding ~= nil then bindingSection:set(id, nil) end
                recording = {
                    id = id,
                    arg = arg,
                    refresh = function() set(id) end,
                }
                recording.refresh()
            end),
        },
    }

    local row = {
        type = ui.TYPE.Flex,
        props = {
            horizontal = true,
        },
        content = ui.content {
            name,
            { props = { size = util.vector2(10, 0) } },
            recorder,
        },
    }
    local column = {
        type = ui.TYPE.Flex,
        content = ui.content {
            row,
            description,
        },
    }

    return column
end)

local function bindButton(device, button)
    if recording == nil then return end
    local binding = {
        device = device,
        button = button,
        type = recording.arg.type,
        key = recording.arg.key,
    }
    bindingSection:set(recording.id, binding)
    local refresh = recording.refresh
    recording = nil
    refresh()
end

return {
    engineHandlers = {
        onKeyPress = function(key)
            bindButton(key.code ~= input.KEY.Escape and 'keyboard' or nil, key.code)
        end,
        onMouseButtonPress = function(button)
            bindButton('mouse', button)
        end,
        onControllerButtonPress = function(id)
            bindButton('controller', id)
        end,
    }
}
