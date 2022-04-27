local ui = require('openmw.ui')
local async = require('openmw.async')
local util = require('openmw.util')

local common = require('scripts.omw.settings.common')
local render = require('scripts.omw.settings.render')

render.registerRenderer('text', function(value, set, arg)
    return {
        type = ui.TYPE.TextEdit,
        props = {
            size = util.vector2(arg and arg.size or 150, 30),
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
        registerPage = render.registerPage,
        registerRenderer = render.registerRenderer,
        registerGroup = common.registerGroup,
    },
    engineHandlers = {
        onLoad = common.onLoad,
        onSave = common.onSave,
    },
}