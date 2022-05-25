local util = require('openmw.util')
local ui = require('openmw.ui')

local constants = require('scripts.omw.mwui.constants')

local borderOffset = util.vector2(1, 1) * constants.border

return function(templates)
    local borderContent = ui.content {
        {
            template = templates.borders,
            props = {
                relativeSize = util.vector2(1, 1),
            },
            content = ui.content {
                {
                    props = {
                        position = borderOffset,
                        relativeSize = util.vector2(1, 1),
                    },
                    external = {
                        slot = true,
                    },
                }
            }
        },
    }

    templates.textEditLine = {
        type = ui.TYPE.TextEdit,
        props = {
            size = util.vector2(150, constants.textNormalSize) + borderOffset * 4,
            textSize = constants.textNormalSize,
            textColor = constants.normalColor,
            multiline = false,
        },
        content = borderContent,
    }

    templates.textEditBox = {
        type = ui.TYPE.TextEdit,
        props = {
            size = util.vector2(150, 5 * constants.textNormalSize) + borderOffset * 4,
            textSize = constants.textNormalSize,
            textColor = constants.normalColor,
            multiline = true,
            wordWrap = true,
        },
        content = borderContent,
    }
end