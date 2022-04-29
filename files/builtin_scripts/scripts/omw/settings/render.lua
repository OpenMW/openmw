local ui = require('openmw.ui')
local util = require('openmw.util')
local async = require('openmw.async')
local core = require('openmw.core')
local storage = require('openmw.storage')

local common = require('scripts.omw.settings.common')

local renderers = {}
local function registerRenderer(name, renderFunction)
    renderers[name] = renderFunction
end

local groups = {}
local pageOptions = {}

local padding = function(size)
    return {
        props = {
            size = util.vector2(size, size),
        }
    }
end
local smallPadding = padding(10)
local bigPadding = padding(25)

local pageHeader = {
    props = {
        textColor = util.color.rgb(1, 1, 1),
        textSize = 30,
    },
}
local groupHeader = {
    props = {
        textColor = util.color.rgb(1, 1, 1),
        textSize = 25,
    },
}
local normal = {
    props = {
        textColor = util.color.rgb(1, 1, 1),
        textSize = 20,
    },
}

local function renderSetting(group, setting, value, global)
    local renderFunction = renderers[setting.renderer]
    if not renderFunction then
        error(('Setting %s of %s has unknown renderer %s'):format(setting.key, group.key, setting.renderer))
    end
    local set = function(value)
        if global then
            core.sendGlobalEvent(common.setGlobalEvent, {
                groupKey = group.key,
                settingKey = setting.key,
                value = value,
            })
        else
            storage.playerSection(group.key):set(setting.key, value)
        end
    end
    local l10n = core.l10n(group.l10n)
    return {
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
                            text = l10n(setting.name),
                        },
                    },
                    smallPadding,
                    renderFunction(value, set, setting.argument),
                    smallPadding,
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
        },
    }
end

local groupLayoutName = function(key, global)
    return ('%s%s'):format(global and 'global_' or 'player_', key)
end

local function renderGroup(group, global)
    local l10n = core.l10n(group.l10n)
    local layout = {
        name = groupLayoutName(group.key, global),
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
                        name = 'name',
                        type = ui.TYPE.Text,
                        template = groupHeader,
                        props = {
                            text = l10n(group.name),
                        },
                    },
                    smallPadding,
                    {
                        name = 'description',
                        type = ui.TYPE.Text,
                        template = normal,
                        props = {
                            text = l10n(group.description),
                        },
                    },
                },
            },
            smallPadding,
            {
                name = 'settings',
                type = ui.TYPE.Flex,
                content = ui.content{},
            },
            bigPadding,
        },
    }
    local settingsContent = layout.content.settings.content
    local valueSection = common.getSection(global, group.key)
    for _, setting in pairs(group.settings) do
        settingsContent:add(renderSetting(group, setting, valueSection:get(setting.key), global))
    end
    return layout
end

local function renderPage(page)
    local l10n = core.l10n(page.l10n)
    local layout = {
        name = page.key,
        type = ui.TYPE.Flex,
        content = ui.content {
            smallPadding,
            {
                type = ui.TYPE.Flex,
                props = {
                    horizontal = true,
                    align = ui.ALIGNMENT.Start,
                    arrange = ui.ALIGNMENT.End,
                },
                content = ui.content {
                    {
                        name = 'name',
                        type = ui.TYPE.Text,
                        template = pageHeader,
                        props = {
                            text = l10n(page.name),
                        },
                    },
                    smallPadding,
                    {
                        name = 'description',
                        type = ui.TYPE.Text,
                        template = normal,
                        props = {
                            text = l10n(page.description),
                        },
                    },
                },
            },
            bigPadding,
            {
                name = 'groups',
                type = ui.TYPE.Flex,
                content = ui.content {},
            },
        },
    }
    local groupsContent = layout.content.groups.content
    for _, pageGroup in ipairs(groups[page.key]) do
        local group = common.getSection(pageGroup.global, common.groupSectionKey):get(pageGroup.key)
        groupsContent:add(renderGroup(group, pageGroup.global))
    end
    return {
        name = l10n(page.name),
        element = ui.create(layout),
        searchHints = '',
    }
end

local function onSettingChanged(global)
    return async:callback(function(groupKey, settingKey)
        local group = common.getSection(global, common.groupSectionKey):get(groupKey)
        if not pageOptions[group.page] then return end

        local element = pageOptions[group.page].element
        local groupLayout = element.layout.content.groups.content[groupLayoutName(group.key, global)]
        local settingsLayout = groupLayout.content.settings
        local value = common.getSection(global, group.key):get(settingKey)
        settingsLayout.content[settingKey] = renderSetting(group, group.settings[settingKey], value, global)
        element:update()
    end)
end
local function onGroupRegistered(global, key)
    local group = common.getSection(global, common.groupSectionKey):get(key)
    groups[group.page] = groups[group.page] or {}
    table.insert(groups[group.page], {
        key = group.key,
        global = global,
    })
    common.getSection(global, group.key):subscribe(onSettingChanged(global))

    if not pageOptions[group.page] then return end
    local element = pageOptions[group.page].element
    local groupsLayout = element.layout.content.groups
    -- TODO: make group order deterministic
    groupsLayout.content:add(renderGroup(group, global))
    element:update()
end
local globalGroups = storage.globalSection(common.groupSectionKey)
for groupKey in pairs(globalGroups:asTable()) do
    onGroupRegistered(true, groupKey)
end
globalGroups:subscribe(async:callback(function(_, key)
    if key then onGroupRegistered(true, key) end
end))
storage.playerSection(common.groupSectionKey):subscribe(async:callback(function(_, key)
    if key then onGroupRegistered(false, key) end
end))

local function registerPage(options)
    if type(options) ~= 'table' then
        error('Page options must be a table')
    end
    if type(options.key) ~= 'string' then
        error('Page must have a key')
    end
    if type(options.l10n) ~= 'string' then
        error('Page must have a localization context')
    end
    if type(options.name) ~= 'string' then
        error('Page must have a name')
    end
    if type(options.description) ~= 'string' then
        error('Page must have a description')
    end
    local page = {
        key = options.key,
        l10n = options.l10n,
        name = options.name,
        description = options.description,
    }
    groups[page.key] = groups[page.key] or {}
    pageOptions[page.key] = renderPage(page)
    ui.registerSettingsPage(pageOptions[page.key])
end

return {
    registerPage = registerPage,
    registerRenderer = registerRenderer,
}