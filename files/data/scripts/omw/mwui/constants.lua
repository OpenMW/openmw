local ui = require('openmw.ui')
local util = require('openmw.util')

return {
    textNormalSize = 16,
    textHeaderSize = 16,
    headerColor = util.color.rgb(223 / 255, 201 / 255, 159 / 255),
    normalColor = util.color.rgb(202 / 255, 165 / 255, 96 / 255),
    border = 2,
    thickBorder = 4,
    padding = 2,
    whiteTexture = ui.texture { path = 'white' },
}