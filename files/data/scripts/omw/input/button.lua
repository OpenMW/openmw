local util = require('openmw.util')
local I = require('openmw.interfaces')
local async = require('openmw.async')
local core = require('openmw.core')
local ambient = require('openmw.ambient')
local ui = require('openmw.ui')

local normalColor = util.color.commaString(core.getGMST("FontColor_color_normal"))
local normalColorOver = util.color.commaString(core.getGMST('fontcolor_color_normal_over'))
local normalColorPressed = util.color.commaString(core.getGMST('fontcolor_color_normal_pressed'))

local function paddedBox(content)
    return {
        template = I.MWUI.templates.box,
        content = ui.content {
            {
                template = I.MWUI.templates.padding,
                content = content,
            },
        }
    }
end

local function buttonPress(layout, press)
    layout.userData.pressed = press
    layout.userData:update()
end

local function buttonFocus(layout, focus)
    if layout.userData then
        layout.userData.focused = focus
        layout.userData:update()
    end
end

local buttonEvents = {
    mousePress = async:callback(function(_, layout)
        if layout.userData then
            buttonPress(layout, true)
            ambient.playSound('menu click')
        end
    end),
    mouseClick = async:callback(function(_, layout)
        if layout.userData then
            layout.userData.callback()
        end
    end),
    mouseRelease = async:callback(function(_, layout)
        if layout.userData then
            buttonPress(layout, false)
        end
    end),
    focusGain = async:callback(function(_, layout)
        buttonFocus(layout, true)
        return true
    end),
    focusLoss = async:callback(function(_, layout)
        buttonFocus(layout, false)
        return true
    end),
}

local function newButton(options)
    local widget = {
        text = options.text,
        callback = options.callback,
    }
    assert(widget.callback)

    function widget:update()
        local textProps = widget.textContent[1].props
        textProps.text = self.text
        textProps.textColor = widget:color()
        self.element:update()
    end

    function widget:color()
        if self.pressed then
            return normalColorPressed
        elseif self.focused then
            return normalColorOver
        end
        return normalColor
    end

    widget.textContent = ui.content{{
        name = 'text',
        template = I.MWUI.templates.textNormal,
        props = {},
    }, {
        -- Include a blank widget to ensure a minimum size
        type = ui.TYPE.Widget,
        props = {size = options.minimumSize}
    }
    }

    widget.element = ui.create{
        type = ui.TYPE.Container,
        content = ui.content{paddedBox(widget.textContent)},
        props = options.props,
        userData = widget,
        events = buttonEvents,
    }
    widget:update()
    return widget.element
end
return newButton
