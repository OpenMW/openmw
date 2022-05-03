local ui = require('openmw.ui')
local util = require('openmw.util')

local whiteTexture = ui.texture{ path = 'white' }

local menuTransparency = ui._getMenuTransparency()

return function(templates)
    templates.backgroundTransparent = {
        props = {
            resource = whiteTexture,
            color = util.color.rgb(0, 0, 0),
            alpha = menuTransparency,
        },
    }
    templates.backgroundSolid = {
        props = {
            resource = whiteTexture,
            color = util.color.rgb(0, 0, 0),
        },
    }
    templates.box = {
        props = {
            inheritAlpha = false,
        },
        content = ui.content {
            {
                type = ui.TYPE.Image,
                template = templates.backgroundTransparent,
                props = {
                    relativeSize = util.vector2(1, 1),
                },
            },
            {
                template = templates.borders,
                props = {
                    relativeSize = util.vector2(1, 1),
                },
                external = {
                    slot = true,
                },
            },
        },
    }
end