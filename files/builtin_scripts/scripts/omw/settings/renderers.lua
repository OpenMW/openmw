local ui = require('openmw.ui')
local async = require('openmw.async')
local I = require('openmw.interfaces')

return function(registerRenderer)
    registerRenderer('textLine', function(value, set)
        return {
            template = I.MWUI.templates.textEditLine,
            props = {
                text = tostring(value),
            },
            events = {
                textChanged = async:callback(function(s) set(s) end),
            },
        }
    end)

    registerRenderer('yesNo', function(value, set)
        return {
            template = I.MWUI.templates.box,
            content = ui.content {
                {
                    template = I.MWUI.templates.padding,
                    content = ui.content {
                        {
                            template = I.MWUI.templates.textNormal,
                            props = {
                                text = value and 'Yes' or 'No',
                            },
                            events = {
                                mouseClick = async:callback(function() set(not value) end),
                            },
                        },
                    },
                },
            },
        }
    end)
end