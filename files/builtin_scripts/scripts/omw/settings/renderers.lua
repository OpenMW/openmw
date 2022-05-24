local core = require('openmw.core')
local ui = require('openmw.ui')
local async = require('openmw.async')
local util = require('openmw.util')
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
            l10n = 'Interface',
            trueLabel = 'Yes',
            falseLabel = 'No',
        }
        registerRenderer('checkbox', function(value, set, argument)
            argument = applyDefaults(argument, defaultArgument)
            local l10n = core.l10n(argument.l10n)
            return disable(argument.disabled, {
                template = I.MWUI.templates.box,
                content = ui.content {
                    {
                        template = I.MWUI.templates.padding,
                        content = ui.content {
                            {
                                template = I.MWUI.templates.textNormal,
                                props = {
                                    text = l10n(value and argument.trueLabel or argument.falseLabel)
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

    do
        local function validateNumber(text, argument)
            local number = tonumber(text)
            if not number then return end
            if argument.min and number < argument.min then return end
            if argument.max and number > argument.max then return end
            if argument.integer and math.floor(number) ~= number then return end
            return number
        end
        local defaultArgument = {
            disabled = false,
            integer = false,
            min = nil,
            max = nil,
        }
        registerRenderer('number', function(value, set, argument)
            argument = applyDefaults(argument, defaultArgument)
            local lastInput = nil
            return disable(argument.disabled, {
                template = I.MWUI.templates.textEditLine,
                props = {
                    text = tostring(value),
                },
                events = {
                    textChanged = async:callback(function(text)
                        lastInput = text
                    end),
                    focusLoss = async:callback(function()
                        if not lastInput then return end
                        local number = validateNumber(lastInput, argument)
                        if not number then
                            set(value)
                        end
                        if number and number ~= value then
                            set(number)
                        end
                    end),
                },
            })
        end)
    end

    do
        local defaultArgument = {
            disabled = false,
            l10n = nil,
            items = {},
        }
        local leftArrow = ui.texture {
            path = 'textures/omw_menu_scroll_left.dds',
        }
        local rightArrow = ui.texture {
            path = 'textures/omw_menu_scroll_right.dds',
        }
        registerRenderer('select', function(value, set, argument)
            argument = applyDefaults(argument, defaultArgument)
            if not argument.l10n then
                error('"select" renderer requires a "l10n" argument')
            end
            local l10n = core.l10n(argument.l10n)
            local index = nil
            local itemCount = 0
            for i, item in ipairs(argument.items) do
                itemCount = itemCount + 1
                if item == value then
                    index = i
                end
            end
            if not index then return {} end
            local label = l10n(value)
            local body = {
                type = ui.TYPE.Flex,
                props = {
                    horizontal = true,
                    arrange = ui.ALIGNMENT.Center,
                },
                content = ui.content {
                    {
                        type = ui.TYPE.Image,
                        props = {
                            resource = leftArrow,
                            size = util.vector2(1, 1) * 12,
                        },
                        events = {
                            mouseClick = async:callback(function()
                                index = (index - 2) % itemCount + 1
                                set(argument.items[index])
                            end),
                        },
                    },
                    {
                        template = I.MWUI.templates.textNormal,
                        props = {
                            text = label,
                        },
                        external = {
                            grow = 1,
                        },
                    },
                    {
                        type = ui.TYPE.Image,
                        props = {
                            resource = rightArrow,
                            size = util.vector2(1, 1) * 12,
                        },
                        events = {
                            mouseClick = async:callback(function()
                                index = (index) % itemCount + 1
                                set(argument.items[index])
                            end),
                        },
                    },
                },
            }
            return disable(argument.disabled, {
                template = I.MWUI.templates.box,
                content = ui.content {
                    {
                        template = I.MWUI.templates.padding,
                        content = ui.content {
                            body,
                        },
                    },
                },
            })
        end)
    end

    do
        local whiteTexture = ui.texture { path = 'white' }
        local defaultArgument = {
            disabled = false,
        }
        registerRenderer('color', function(value, set, argument)
            argument = applyDefaults(argument, defaultArgument)
            local colorDisplay = {
                template = I.MWUI.templates.box,
                content = ui.content {
                    {
                        type = ui.TYPE.Image,
                        props = {
                            resource = whiteTexture,
                            color = value,
                            -- TODO: remove hardcoded size when possible
                            size = util.vector2(1, 1) * 20,
                        },
                    }
                },
            }
            local lastInput = nil
            local hexInput = {
                template = I.MWUI.templates.textEditLine,
                props = {
                    text = value:asHex(),
                },
                events = {
                    textChanged = async:callback(function(text)
                        lastInput = text
                    end),
                    focusLoss = async:callback(function()
                        if not lastInput then return end
                        if not pcall(function() set(util.color.hex(lastInput)) end)
                        then set(value) end
                    end),
                },
            }
            return disable(argument.disabled, {
                type = ui.TYPE.Flex,
                props = {
                    horizontal = true,
                    arrange = ui.ALIGNMENT.Center,
                },
                content = ui.content {
                    colorDisplay,
                    { template = I.MWUI.templates.interval },
                    hexInput,
                }
            })
        end)
    end
end