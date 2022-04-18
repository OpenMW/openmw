local ui = require('openmw.ui')
local util = require('openmw.util')
local async = require('openmw.async')
local core = require('openmw.core')

local common = require('scripts.omw.settings.common')

local renderers = {}
local function registerRenderer(name, renderFunction)
    renderers[name] = renderFunction
end

local groupOptions = {}


local padding = function(size)
    return {
        props = {
            size = util.vector2(size, size),
        }
    }
end

local header = {
    props = {
        textColor = util.color.rgb(1, 1, 1),
        textSize = 30,
    },
}

local normal = {
    props = {
        textColor = util.color.rgb(1, 1, 1),
        textSize = 25,
    },
}

local function renderSetting(groupKey, setting, value)
    local renderFunction = renderers[setting.renderer]
    if not renderFunction then
        error(('Setting %s of %s has unknown renderer %s'):format(setting.key, groupKey, setting.renderer))
    end
    local group = common.group(groupKey)
    value = value or group[setting.key]
    local set = function(value)
        group[setting.key] = value
        renderSetting(groupKey, setting, value)
    end
    local element = groupOptions[groupKey].element
    local localization = groupOptions[groupKey].localization
    local settingsLayout = element.layout.content.settings
    settingsLayout.content[setting.key] = {
        name = setting.key,
        type = ui.TYPE.Flex,
        content = ui.content {
            {
                type = ui.TYPE.Flex,
                props = {
                    horizontal = true,
                    align = ui.ALIGNMENT.Start,
                    arrange = ui.ALIGNMENT.End,
                },
                content = ui.content {
                    {
                        type = ui.TYPE.Text,
                        template = normal,
                        props = {
                            text = localization(setting.name),
                        },
                    },
                    padding(10),
                    renderFunction(value, set, setting.argument),
                    padding(10),
                    {
                        type = ui.TYPE.Text,
                        template = normal,
                        props = {
                            text = 'Reset',
                        },
                        events = {
                            mouseClick = async:callback(function()
                                set(setting.default)
                            end),
                        },
                    },
                },
            },
            padding(20),
        },
    }
    element:update()
end

local function renderGroup(groupKey)
    local group = common.groups():get(groupKey)
    local element = groupOptions[groupKey].element
    local localization = groupOptions[groupKey].localization
    element.layout = {
        type = ui.TYPE.Flex,
        content = ui.content {
            padding(10),
            {
                type = ui.TYPE.Flex,
                props = {
                    horizontal = true,
                    align = ui.ALIGNMENT.Start,
                    arrange = ui.ALIGNMENT.Center,
                },
                content = ui.content {
                    {
                        name = 'name',
                        type = ui.TYPE.Text,
                        template = header,
                        props = {
                            text = localization(group.name),
                        },
                    },
                    padding(10),
                    {
                        name = 'description',
                        type = ui.TYPE.Text,
                        template = normal,
                        props = {
                            text = localization(group.description),
                        },
                    },
                },
            },
            padding(50),
            {
                name = 'settings',
                type = ui.TYPE.Flex,
                content = ui.content{},
            },
        },
    }
    local settingsContent = element.layout.content.settings.content
    for _, setting in pairs(group.settings) do
        settingsContent:add({ name = setting.key })
        renderSetting(groupKey, setting)
    end
    element:update()
end

local function onGroupRegistered(groupKey)
    local group = common.groups():get(groupKey)
    local loc = core.l10n(group.localization)
    local options = {
        name = loc(group.name),
        element = ui.create{},
        searchHints = '',
        localization = loc,
    }
    groupOptions[groupKey] = options
    renderGroup(groupKey)
    ui.registerSettingsPage(options)
end

local function onSettingChanged(event)
    local group = common.groups():get(event.groupKey)
    local setting = group.settings[event.settingKey]
    renderSetting(event.groupKey, setting, event.value)
end

return {
    onGroupRegistered = onGroupRegistered,
    onSettingChanged = onSettingChanged,
    registerRenderer = registerRenderer,
}