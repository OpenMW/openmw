local ui = require("openmw.ui")
local util = require("openmw.util")


ui.layers.insertAfter("MainMenuBackground", "builtin:map_extractor", {interactive = false})

local content = ui.content{
    {
        type = ui.TYPE.Text,
        props = {
            text = "",
            textSize = 20,
            autoSize = true,
            textColor = util.color.rgb(1, 1, 1),
            textAlignH = ui.ALIGNMENT.Center,
            textAlignV = ui.ALIGNMENT.Center,
            textShadow = true,
            textShadowColor = util.color.rgb(0, 0, 0),
            anchor = util.vector2(0.5, 0.5),
        },
    },
    {
        type = ui.TYPE.Text,
        props = {
            text = "",
            textSize = 20,
            autoSize = true,
            textColor = util.color.rgb(1, 1, 1),
            textAlignH = ui.ALIGNMENT.Center,
            textAlignV = ui.ALIGNMENT.Center,
            textShadow = true,
            textShadowColor = util.color.rgb(0, 0, 0),
            anchor = util.vector2(0.5, 0.5),
        },
    }
}


local layout = {
    type = ui.TYPE.Container,
    layer = "builtin:map_extractor",
    props = {
        anchor = util.vector2(0.5, 0.5),
        relativePosition = util.vector2(0.5, 0.5),
    },
    content = ui.content{
        {
            type = ui.TYPE.Flex,
            props = {
                align = ui.ALIGNMENT.Center,
                arrange = ui.ALIGNMENT.Center,
            },
            content = content,
        }
    },
}


local menu = ui.create(layout)


return {
    eventHandlers = {
        ["builtin:map_extractor:updateMenu"] = function (data)
            if not data then data = {} end

            if data.line1 then
                content[1].props.text = data.line1
            end
            if data.line2 then
                content[2].props.text = data.line2
            end

            menu:update()
        end,
    }
}