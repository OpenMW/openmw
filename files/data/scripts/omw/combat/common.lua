local async = require('openmw.async')
local storage = require('openmw.storage')
local I = require('openmw.interfaces')

local combatGroup = 'SettingsOMWCombat'

return {
    registerSettingsPage = function()
        I.Settings.registerPage({
          key = 'OMWCombat',
          l10n = 'OMWCombat',
          name = 'Combat',
          description = 'combatSettingsPageDescription',
        })
    end,
    registerSettingsGroup = function()
        local function boolSetting(key, default)
            return {
                key = key,
                renderer = 'checkbox',
                name = key,
                description = key..'Description',
                default = default,
            }
        end

        I.Settings.registerGroup({
            key = combatGroup,
            page = 'OMWCombat',
            l10n = 'OMWCombat',
            name = 'combatSettings',
            permanentStorage = false,
            order = 0,
            settings = {
                boolSetting('unarmedCreatureAttacksDamageArmor', false),
                boolSetting('redistributeShieldHitsWhenNotWearingShield', false),
                boolSetting('spawnBloodEffectsOnPlayer', false),
            },
        })
    end,
}
