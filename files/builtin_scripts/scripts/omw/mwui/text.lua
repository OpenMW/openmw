local ui = require('openmw.ui')

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

return function(templates)
    templates.textNormal = textNormal
    templates.textHeader = textHeader
end