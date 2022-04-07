local ui = require('openmw.ui')
local util = require('openmw.util')

local common = require('scripts.omw.settings.common')

local renderers = {}
local function registerRenderer(name, renderFunction)
    renderers[name] = renderFunction
end

local groupOptions = {}

local function renderSetting(groupName, setting, value, index)
    local renderFunction = renderers[setting.renderer]
    if not renderFunction then
        error(('Setting %s of %s has unknown renderer %s'):format(setting.name, groupName, setting.renderer))
    end
    local layout = renderFunction(setting, value or setting.default, function(value)
        local group = common.getGroup(groupName)
        group:set(setting.name, value)
        local element = groupOptions[groupName].element
        local settingLayout = renderSetting(groupName, setting, value, index)
        settingLayout.name = setting.name
        element.layout.content[setting.name] = settingLayout
        element:update()
    end)
    layout.name = setting.name
    return layout
end

local function onGroupRegistered(groupName)
    local group = common.groups:get(groupName)
    local layout = {
        type = ui.TYPE.Flex,
        content = ui.content{},
    }
    local searchHints = { groupName }
    local count = 0
    for _, setting in pairs(group) do
        count = count + 1
        local settingLayout = renderSetting(groupName, setting, setting.default, count)
        settingLayout.name = setting.name
        layout.content:add(settingLayout)
        table.insert(searchHints, setting.name)
    end
    local options = {
        name = groupName,
        element = ui.create(layout),
        searchHints = table.concat(searchHints, ' '),
    }
    groupOptions[groupName] = options
    print(('registering group %s'):format(groupName))
    ui.registerSettingsPage(options)
end

return {
    onGroupRegistered = onGroupRegistered,
    registerRenderer = registerRenderer,
}