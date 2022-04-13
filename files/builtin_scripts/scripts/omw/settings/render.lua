local ui = require('openmw.ui')
local util = require('openmw.util')
local async = require('openmw.async')

local common = require('scripts.omw.settings.common')

local renderers = {}
local function registerRenderer(name, renderFunction)
    renderers[name] = renderFunction
end

local groupOptions = {}
local localization = {}

local function renderSetting(groupKey, setting, value)
    local renderFunction = renderers[setting.renderer]
    if not renderFunction then
        error(('Setting %s of %s has unknown renderer %s'):format(setting.key, groupKey, setting.renderer))
    end
    local settingName = localization[groupKey]
        and localization[groupKey].settings[setting.key].name
        or setting.key
    local set = function(value)
        local group = common.getGroup(groupKey)
        group:set(setting.key, value)
        renderSetting(groupKey, setting, value)
    end
    local element = groupOptions[groupKey].element
    local settingsLayout = element.layout.content.settings
    settingsLayout.content[setting.key] = {
        name = setting.key,
        type = ui.TYPE.Flex,
        props = {
            horizontal = true,
            align = ui.ALIGNMENT.Start,
            arrange = ui.ALIGNMENT.Center,
        },
        content = ui.content {
            {
                type = ui.TYPE.Text,
                props = {
                    text = settingName .. ':',
                    textColor = util.color.rgb(1, 1, 1),
                    textSize = 30,
                },
            },
            renderFunction(value or setting.default, set, setting.argument),
            {
                type = ui.TYPE.Text,
                props = {
                    text = 'Reset',
                    textColor = util.color.rgb(1, 1, 1),
                    textSize = 30,
                },
                events = {
                    mouseClick = async:callback(function()
                        set(setting.default)
                    end),
                },
            },
        },
    }
    element:update()
end

local function updateLocalization(groupKey)
    local loc = localization[groupKey]
    local options = groupOptions[groupKey]
    if not options or not loc then return end
    local searchHints = { loc.name, loc.description }
    options.name = loc.name
    options.searchHints = table.concat(searchHints, ' ')
    local layout = options.element.layout
    layout.content.header.props.text = loc.description
end

local function onGroupRegistered(groupKey)
    local group = common.groups:get(groupKey)
    local layout = {
        type = ui.TYPE.Flex,
        content = ui.content {
            {
                name = 'header',
                type = ui.TYPE.Text,
                props = {
                    text = '',
                    textSize = 30,
                    textColor = util.color.rgb(1, 1, 1),
                },
            },
            {
                name = 'settings',
                type = ui.TYPE.Flex,
                content = ui.content{},
            },
        },
    }
    local options = {
        name = groupKey,
        element = ui.create(layout),
        searchHints = '',
    }
    groupOptions[groupKey] = options
    for _, setting in pairs(group) do
        layout.content.settings.content:add({ name = setting.key })
        renderSetting(groupKey, setting, setting.default)
    end
    updateLocalization(groupKey)
    print(('registering group %s'):format(groupKey))
    ui.registerSettingsPage(options)
end

local function localizeGroup(groupKey, loc)
    localization[groupKey] = loc
    updateLocalization(groupKey)
end

return {
    onGroupRegistered = onGroupRegistered,
    registerRenderer = registerRenderer,
    localizeGroup = localizeGroup,
}