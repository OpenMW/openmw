local common = require('scripts.omw.settings.common')
local render = require('scripts.omw.settings.render')

local saveScope = common.scopes[common.SCOPE.SavePlayer]
return {
    interfaceName = 'Settings',
    interface = {
        SCOPE = common.SCOPE,
        getGroup = common.getGroup,
        registerRenderer = render.registerRenderer,
        localizeGroup = render.localizeGroup,
    },
    engineHandlers = {
        onLoad = function(saved)
            saveScope:reset(saved)
        end,
        onSave = function()
            return saveScope:asTable()
        end,
    },
    eventHandlers = {
        [common.EVENTS.SettingChanged] = function(e)
            common.getGroup(e.groupName):__changed(e.settingName, e.value)
        end,
        [common.EVENTS.GroupRegistered] = render.onGroupRegistered,
    }
}