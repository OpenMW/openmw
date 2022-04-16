local common = require('scripts.omw.settings.common')
local render = require('scripts.omw.settings.render')
local ui = require('openmw.ui')
local async = require('openmw.async')
local util = require('openmw.util')

render.registerRenderer('text', function(value, set, arg)
    return {
        type = ui.TYPE.TextEdit,
        props = {
            size = util.vector2(arg and arg.size or 300, 30),
            text = value,
            textColor = util.color.rgb(1, 1, 1),
            textSize = 15,
            textAlignV = ui.ALIGNMENT.Center,
        },
        events = {
            textChanged = async:callback(function(s) set(s) end),
        },
    }
end)

local saveScope = common.scopes[common.SCOPE.SavePlayer]
return {
    interfaceName = 'Settings',
    interface = {
        SCOPE = common.SCOPE,
        getGroup = common.getGroup,
        registerRenderer = render.registerRenderer,
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