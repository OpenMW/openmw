local common = require('scripts.omw.settings.common')
local render = require('scripts.omw.settings.render')

local ui = require('openmw.ui')
local async = require('openmw.async')
local util = require('openmw.util')
local core = require('openmw.core')

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

return {
    interfaceName = 'Settings',
    interface = {
        SCOPE = common.SCOPE,
        group = common.group,
        registerRenderer = render.registerRenderer,
        registerGroup = function(options)
            core.sendGlobalEvent(common.EVENTS.RegisterGroup, options)
        end,
    },
    engineHandlers = {
        onLoad = function(saved)
            common.loadScope(common.SCOPE.SavePlayer, saved)
        end,
        onSave = function()
            common.saveScope(common.SCOPE.SavePlayer)
        end,
    },
    eventHandlers = {
        [common.EVENTS.GroupRegistered] = render.onGroupRegistered,
        [common.EVENTS.SettingChanged] = render.onSettingChanged,
        [common.EVENTS.Subscribe] = common.handleSubscription,
    }
}