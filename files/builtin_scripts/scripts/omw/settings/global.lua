local common = require('scripts.omw.settings.common')
local register = require('scripts.omw.settings.register')
local world = require('openmw.world')
local types = require('openmw.types')

return {
    interfaceName = 'Settings',
    interface = {
        SCOPE = common.SCOPE,
        group = common.group,
        registerGroup = register.registerGroup,
    },
    engineHandlers = {
        onLoad = function(saved)
            common.groups():reset()
            common.loadScope(common.SCOPE.SaveGlobal, saved)
        end,
        onSave = function()
            common.saveScope(common.SCOPE.SaveGlobal)
        end,
        onPlayerAdded = register.onPlayerAdded,
    },
    eventHandlers = {
        [common.EVENTS.SetValue] = function(event)
            local group = common.group(event.groupKey)
            group[event.settingKey] = event.value
        end,
        [common.EVENTS.RegisterGroup] = function(options)
            if common.groups():get(options.key) then return end
            register.registerGroup(options)
        end,
        [common.EVENTS.SettingChanged] = function(event)
            local setting = common.groups():get(event.groupKey).settings[event.settingKey]
            if common.isPlayerScope(setting.scope) then
                for _, a in ipairs(world.activeActors) do
                    if a.type == types.Player and a:isValid() then
                        a:sendEvent(common.EVENTS.SettingChanged, event)
                    end
                end
            end
        end,
        [common.EVENTS.Subscribe] = common.handleSubscription,
    },
}