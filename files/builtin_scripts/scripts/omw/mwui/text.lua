local ui = require('openmw.ui')

local constants = require('scripts.omw.mwui.constants')

local textNormal = {
    type = ui.TYPE.Text,
    props = {
        textSize = constants.textNormalSize,
        textColor = constants.sandColor,
    },
}

return function(templates)
    templates.textNormal = textNormal
end