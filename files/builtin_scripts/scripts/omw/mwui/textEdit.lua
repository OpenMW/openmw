local util = require('openmw.util')
local ui = require('openmw.ui')

local constants = require('scripts.omw.mwui.constants')

return function(templates)
    local borderContent = ui.content {
        {
            template = templates.borders,
            props = {
                relativeSize = util.vector2(1, 1),
            },
            content = ui.content {
                {
                    external = {
                        slot = true,
                    },
                },
            }
        },
    }

    templates.textEditLine = {
        type = ui.TYPE.TextEdit,
        props = {
            textSize = constants.textNormalSize,
            textColor = constants.sandColor,
            textAlignH = ui.ALIGNMENT.Start,
            textAlignV = ui.ALIGNMENT.Center,
            multiline = false,
        },
        content = borderContent,
    }

    templates.textEditBox = {
        type = ui.TYPE.TextEdit,
        props = {
            textSize = constants.textNormalSize,
            textColor = constants.sandColor,
            textAlignH = ui.ALIGNMENT.Start,
            textAlignV = ui.ALIGNMENT.Start,
            multiline = true,
            wordWrap = true,
        },
        content = borderContent,
    }
end