local ui = require('openmw.ui')
local util = require('openmw.util')

local constants = require('scripts.omw.mwui.constants')

local textNormal = {
    type = ui.TYPE.Text,
    props = {
        textSize = constants.textNormalSize,
        textColor = constants.normalColor,
    },
}

local textHeader = {
    type = ui.TYPE.Text,
    props = {
        textSize = constants.textHeaderSize,
        textColor = constants.headerColor,
    },
}

local textParagraph = {
    type = ui.TYPE.TextEdit,
    props = {
        textSize = constants.textNormalSize,
        textColor = constants.normalColor,
        autoSize = true,
        readOnly = true,
        multiline = true,
        wordWrap = true,
        size = util.vector2(100, 0),
    },
}

return function(templates)
    templates.textNormal = textNormal
    templates.textHeader = textHeader
    templates.textParagraph = textParagraph
end