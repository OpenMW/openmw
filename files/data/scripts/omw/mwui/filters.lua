local ui = require('openmw.ui')
local util = require('openmw.util')

local constants = require('scripts.omw.mwui.constants')

return function(templates)
    templates.disabled = {
        type = ui.TYPE.Container,
        props = {
            alpha = 0.6,
        },
        content = ui.content {
            {
                props = {
                    relativeSize = util.vector2(1, 1),
                },
                external = {
                    slot = true,
                },
            },
            {
                type = ui.TYPE.Image,
                props = {
                    resource = constants.whiteTexture,
                    color = util.color.rgb(0, 0, 0),
                    relativeSize = util.vector2(1, 1),
                },
            },
        },
    }
end