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


I.Settings.registerRenderer('inputBinding', function(id, set, arg)
    if type(id) ~= 'string' then error('inputBinding: must have a string default value') end
    if not arg then error('inputBinding: argument with "key" and "type" is required') end
    if not arg.type then error('inputBinding: type argument is required') end
    if not arg.key then error('inputBinding: key argument is required') end
    local info = input.actions[arg.key] or input.triggers[arg.key]
    if not info then return {} end

    local l10n = core.l10n(info.key)

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
    local label = interfaceL10n('None')
    if binding then label = input.getKeyName(binding.code) end
    if recording and recording.id == id then label = interfaceL10n('N/A') end

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

return {
    engineHandlers = {
        onKeyPress = function(key)
            if recording == nil then return end
            local binding = {
                code = key.code,
                type = recording.arg.type,
                key = recording.arg.key,
            }
            if key.code == input.KEY.Escape then -- TODO: prevent settings modal from closing
                binding.code = nil
            end
            bindingSection:set(recording.id, binding)
            local refresh = recording.refresh
            recording = nil
            refresh()
        end,
    }
}
