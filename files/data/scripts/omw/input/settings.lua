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
