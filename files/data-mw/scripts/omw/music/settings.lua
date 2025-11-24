local I = require('openmw.interfaces')
local storage = require('openmw.storage')

I.Settings.registerPage({
    key = 'OMWMusic',
    l10n = 'OMWMusic',
    name = 'Music',
    description = 'settingsPageDescription',
})

I.Settings.registerGroup({
    key = "SettingsOMWMusic",
    page = 'OMWMusic',
    l10n = 'OMWMusic',
    name = 'musicSettings',
    permanentStorage = true,
    order = 0,
    settings = {
        {
            key = 'CombatMusicEnabled',
            renderer = 'checkbox',
            name = 'CombatMusicEnabled',
            description = 'CombatMusicEnabledDescription',
            default = true,
        }
    },
})
