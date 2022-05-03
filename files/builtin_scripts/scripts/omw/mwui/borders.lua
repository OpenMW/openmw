local ui = require('openmw.ui')
local util = require('openmw.util')

local constants = require('scripts.omw.mwui.constants')

local v2 = util.vector2

local sideParts = {
    left = util.vector2(0, 0.5),
    right = util.vector2(1, 0.5),
    top = util.vector2(0.5, 0),
    bottom = util.vector2(0.5, 1),
}
local cornerParts = {
    top_left_corner = util.vector2(0, 0),
    top_right_corner = util.vector2(1, 0),
    bottom_left_corner = util.vector2(0, 1),
    bottom_right_corner = util.vector2(1, 1),
}

local resources = {}
do
    local boxBorderPattern = 'textures/menu_thin_border_%s.dds'
    for k, _ in pairs(sideParts) do
        resources[k] = ui.texture{ path = boxBorderPattern:format(k) }
    end
    for k, _ in pairs(cornerParts) do
        resources[k] = ui.texture{ path = boxBorderPattern:format(k) }
    end
end

local borderPieces = {}
for k, align in pairs(sideParts) do
    local resource = resources[k]
    local horizontal = align.x ~= 0.5
    borderPieces[#borderPieces + 1] = {
        type = ui.TYPE.Image,
        props = {
            resource = resource,
            relativePosition = align,
            anchor = align,
            relativeSize = horizontal and v2(0, 1) or v2(1, 0),
            size = (horizontal and v2(1, -2) or v2(-2, 1)) * constants.borderSize,
            tileH = not horizontal,
            tileV = horizontal,
        },
    }
end

for k, align in pairs(cornerParts) do
    local resource = resources[k]
    borderPieces[#borderPieces + 1] = {
        type = ui.TYPE.Image,
        props = {
            resource = resource,
            relativePosition = align,
            anchor = align,
            size = v2(1, 1) * constants.borderSize,
        },
    }
end

borderPieces[#borderPieces + 1] = {
    external = {
        slot = true,
    },
    props = {
        position = v2(1, 1) * (constants.borderSize + constants.padding),
        size = v2(-2, -2) * (constants.borderSize + constants.padding),
        relativeSize = v2(1, 1),
    },
}

local borders = {
    content = ui.content(borderPieces)
}
borders.content:add({
    external = {
        slot = true,
    },
    props = {
        size = v2(-2, -2) * constants.borderSize,
    },
})

local horizontalLine = {
    content = ui.content {
        {
            type = ui.TYPE.Image,
            props = {
                resource = resources.top,
                tileH = true,
                tileV = false,
                size = v2(0, constants.borderSize),
                relativeSize = v2(1, 0),
            },
        },
    },
}

local verticalLine = {
    content = ui.content {
        {
            type = ui.TYPE.Image,
            props = {
                resource = resources.left,
                tileH = false,
                tileV = true,
                size = v2(constants.borderSize, 0),
                relativeSize = v2(0, 1),
            },
        },
    },
}

return function(templates)
    templates.borders = borders
    templates.horizontalLine = horizontalLine
    templates.verticalLine = verticalLine
end