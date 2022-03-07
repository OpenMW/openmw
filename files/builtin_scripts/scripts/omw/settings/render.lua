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
        element.layout.content[setting.name] = renderSetting(groupName, setting, value, index)
        element:update()
    end)
    layout.name = setting.name
    -- temporary hacky position and size
    layout.props = layout.props or {}
    layout.props.position = util.vector2(0, 100 * (index - 1))
    layout.props.size = util.vector2(400, 100)
    return layout
end

local function onGroupRegistered(groupName)
    local group = common.groups:get(groupName)
    local layout = {
        content = ui.content{},
    }
    local searchHints = { groupName }
    local count = 0
    for _, setting in pairs(group) do
        count = count + 1
        layout.content:add(renderSetting(groupName, setting, setting.default, count))
        table.insert(searchHints, setting.name)
    end
    layout.props = {
        size = util.vector2(400, 100 * count)
    }
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