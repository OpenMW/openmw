local core = require('openmw.core')
local ui = require('openmw.ui')
local util = require('openmw.util')

return {
    textNormalSize = ui._getDefaultFontSize(),
    textHeaderSize = ui._getDefaultFontSize(),
    headerColor = util.color.commaString(core.getGMST("FontColor_color_header")),
    normalColor = util.color.commaString(core.getGMST("FontColor_color_normal")),
    border = 2,
    thickBorder = 4,
    padding = 2,
    whiteTexture = ui.texture { path = 'white' },
}