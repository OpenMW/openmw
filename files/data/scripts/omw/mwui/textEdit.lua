local util = require('openmw.util')
local ui = require('openmw.ui')

local constants = require('scripts.omw.mwui.constants')

return function(templates)
    templates.textEditLine = {
        type = ui.TYPE.TextEdit,
        props = {
            size = util.vector2(150, constants.textNormalSize),
            textSize = constants.textNormalSize,
            textColor = constants.normalColor,
            multiline = false,
        },
    }

    templates.textEditBox = {
        type = ui.TYPE.TextEdit,
        props = {
            size = util.vector2(150, 5 * constants.textNormalSize),
            textSize = constants.textNormalSize,
            textColor = constants.normalColor,
            multiline = true,
            wordWrap = true,
        },
    }
end