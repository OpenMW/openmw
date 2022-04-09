local ui = require('openmw.ui')
local util = require('openmw.util')

local common = require('scripts.omw.settings.common')

local renderers = {}
local function registerRenderer(name, renderFunction)
    renderers[name] = renderFunction
end

local groupOptions = {}
local localization = {}

local function renderSetting(groupKey, setting, value, index)
    local renderFunction = renderers[setting.renderer]
    if not renderFunction then
        error(('Setting %s of %s has unknown renderer %s'):format(setting.key, groupKey, setting.renderer))
    end
    local loc = localization[groupKey] and localization[groupKey].settings[setting.key] or {
        name = setting.key,
        description = '',
    }
    local layout = renderFunction(loc, value or setting.default, function(value)
        local group = common.getGroup(groupKey)
        group:set(setting.key, value)
        local element = groupOptions[groupKey].element
        local settingLayout = renderSetting(groupKey, setting, value, index)
        settingLayout.name = setting.key
        element.layout.content.settings.content[setting.key] = settingLayout
        element:update()
    end)
    layout.name = setting.key
    return layout
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
    local settingsLayout = {
        name = 'settings',
        type = ui.TYPE.Flex,
        content = ui.content{},
    }
    local count = 0
    for _, setting in pairs(group) do
        count = count + 1
        local settingLayout = renderSetting(groupKey, setting, setting.default, count)
        settingLayout.key = setting.key
        settingsLayout.content:add(settingLayout)
    end
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
            settingsLayout,
        },
    }
    local options = {
        name = groupKey,
        element = ui.create(layout),
        searchHints = '',
    }
    groupOptions[groupKey] = options
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