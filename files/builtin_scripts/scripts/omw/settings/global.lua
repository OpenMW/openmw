local common = require('scripts.omw.settings.common')
local register = require('scripts.omw.settings.register')

local saveScope = common.scopes[common.SCOPE.SaveGlobal]
return {
    interfaceName = 'Settings',
    interface = {
        SCOPE = common.SCOPE,
        getGroup = common.getGroup,
        registerGroup = register.registerGroup,
    },
    engineHandlers = {
        onLoad = function(saved)
            common.groups:reset()
            saveScope:reset(saved)
        end,
        onSave = function()
            return saveScope:asTable()
        end,
        onPlayerAdded = register.onPlayerAdded,
    },
    eventHandlers = {
        [common.EVENTS.SettingChanged] = function(e)
            common.getGroup(e.groupName):__changed(e.settingName, e.value)
        end,
        [common.EVENTS.SettingSet] = function(e)
            common.getGroup(e.groupName):set(e.settingName, e.value)
        end,
    }
}