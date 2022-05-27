local ui = require('openmw.ui')
local util = require('openmw.util')

local constants = require('scripts.omw.mwui.constants')

local borderV = util.vector2(1, 1) * constants.border

return function(templates)
    templates.padding = {
        type = ui.TYPE.Container,
        content = ui.content {
            {
                props = {
                    size = borderV,
                },
            },
            {
                external = { slot = true },
                props = {
                    position = borderV,
                    relativeSize = util.vector2(1, 1),
                },
            },
            {
                props = {
                    position = borderV,
                    relativePosition = util.vector2(1, 1),
                    size = borderV,
                },
            },
        }
    }
    templates.interval = {
        type = ui.TYPE.Widget,
        props = {
            size = borderV,
        },
    }
end