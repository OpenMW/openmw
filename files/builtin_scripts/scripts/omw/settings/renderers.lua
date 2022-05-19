local ui = require('openmw.ui')
local async = require('openmw.async')
local I = require('openmw.interfaces')

local function applyDefaults(argument, defaults)
    if not argument then return defaults end
    if pairs(defaults) and pairs(argument) then
        local result = {}
        for k, v in pairs(defaults) do
            result[k] = v
        end
        for k, v in pairs(argument) do
            result[k] = v
        end
        return result
    end
    return argument
end

local function disable(disabled, layout)
    if disabled then
        return {
            template = I.MWUI.templates.disabled,
            content = ui.content {
                layout,
            },
        }
    else
        return layout
    end
end

return function(registerRenderer)
    do
        local defaultArgument = {
            disabled = false,
        }
        registerRenderer('textLine', function(value, set, argument)
            argument = applyDefaults(argument, defaultArgument)
            return disable(argument.disabled, {
                template = I.MWUI.templates.textEditLine,
                props = {
                    text = tostring(value),
                },
                events = {
                    textChanged = async:callback(function(s) set(s) end),
                },
            })
        end)
    end

    do
        local defaultArgument = {
            disabled = false,
            trueLabel = 'Yes',
            falseLabel = 'No',
        }
        registerRenderer('checkbox', function(value, set, argument)
            argument = applyDefaults(argument, defaultArgument)
            return disable(argument.disabled, {
                template = I.MWUI.templates.box,
                content = ui.content {
                    {
                        template = I.MWUI.templates.padding,
                        content = ui.content {
                            {
                                template = I.MWUI.templates.textNormal,
                                props = {
                                    text = value and argument.trueLabel or argument.falseLabel
                                },
                                events = {
                                    mouseClick = async:callback(function() set(not value) end),
                                },
                            },
                        },
                    },
                },
            })
        end)
    end
end