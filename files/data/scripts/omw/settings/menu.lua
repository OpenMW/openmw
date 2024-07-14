local menu = require('openmw.menu')
local ui = require('openmw.ui')
local util = require('openmw.util')
local async = require('openmw.async')
local core = require('openmw.core')
local storage = require('openmw.storage')
local I = require('openmw.interfaces')

local auxUi = require('openmw_aux.ui')

local common = require('scripts.omw.settings.common')
common.getSection(false, common.groupSectionKey):setLifeTime(storage.LIFE_TIME.GameSession)
-- need to :reset() on reloadlua as well as game session end
common.getSection(false, common.groupSectionKey):reset()

local renderers = {}
local function registerRenderer(name, renderFunction)
    renderers[name] = renderFunction
end
require('scripts.omw.settings.renderers')(registerRenderer)

local interfaceL10n = core.l10n('Interface')

local pages = {}
local groups = {}
local pageOptions = {}
local groupElements = {}

local interval = { template = I.MWUI.templates.interval }
local growingIntreval = {
    template = I.MWUI.templates.interval,
    external = {
        grow = 1,
    },
}
local spacer = {
    props = {
        size = util.vector2(0, 10),
    },
}
local bigSpacer = {
    props = {
        size = util.vector2(0, 50),
    },
}
local stretchingLine = {
    template = I.MWUI.templates.horizontalLine,
    external = {
        stretch = 1,
    },
}
local spacedLines = function(count)
    local content = {}
    table.insert(content, spacer)
    table.insert(content, stretchingLine)
    for _ = 2, count do
        table.insert(content, interval)
        table.insert(content, stretchingLine)
    end
    table.insert(content, spacer)
    return {
        type = ui.TYPE.Flex,
        external = {
            stretch = 1,
        },
        content = ui.content(content),
    }
end

local function interlaceSeparator(layouts, separator)
    local result = {}
    result[1] = layouts[1]
    for i = 2, #layouts do
        table.insert(result, separator)
        table.insert(result, layouts[i])
    end
    return result
end

local function setSettingValue(global, groupKey, settingKey, value)
    if global then
        core.sendGlobalEvent(common.setGlobalEvent, {
            groupKey = groupKey,
            settingKey = settingKey,
            value = value,
        })
    else
        storage.playerSection(groupKey):set(settingKey, value)
    end
end

local function renderSetting(group, setting, value, global)
    local renderFunction = renderers[setting.renderer]
    if not renderFunction then
        error(('Setting %s of %s has unknown renderer %s'):format(setting.key, group.key, setting.renderer))
    end
    local set = function(value)
        setSettingValue(global, group.key, setting.key, value)
    end
    local l10n = core.l10n(group.l10n)
    local titleLayout = {
        type = ui.TYPE.Flex,
        content = ui.content {
            {
                template = I.MWUI.templates.textHeader,
                props = {
                    text = l10n(setting.name),
                },
            },
        },
    }
    if setting.description then
        titleLayout.content:add(interval)
        titleLayout.content:add {
            template = I.MWUI.templates.textParagraph,
            props = {
                text = l10n(setting.description),
                size = util.vector2(300, 0),
            },
        }
    end
    local argument = common.getArgumentSection(global, group.key):get(setting.key)
    local ok, rendererResult = pcall(renderFunction, value, set, argument)
    if not ok then
        print(string.format('Setting %s renderer "%s" error: %s', setting.key, setting.renderer, rendererResult))
    end

    return {
        name = setting.key,
        type = ui.TYPE.Flex,
        props = {
            horizontal = true,
            arrange = ui.ALIGNMENT.Center,
        },
        external = {
            stretch = 1,
        },
        content = ui.content {
            titleLayout,
            growingIntreval,
            ok and rendererResult or {}, -- TODO: display error?
        },
    }
end

local groupLayoutName = function(key, global)
    return ('%s%s'):format(global and 'global_' or 'player_', key)
end

local function renderGroup(group, global)
    local l10n = core.l10n(group.l10n)

    local valueSection = common.getSection(global, group.key)
    local settingLayouts = {}
    local sortedSettings = {}
    for _, setting in pairs(group.settings) do
        sortedSettings[setting.order] = setting
    end
    for _, setting in ipairs(sortedSettings) do
        table.insert(settingLayouts, renderSetting(group, setting, valueSection:get(setting.key), global))
    end
    local settingsContent = ui.content(interlaceSeparator(settingLayouts, spacedLines(1)))

    local resetButtonLayout = {
        template = I.MWUI.templates.box,
        events = {
            mouseClick = async:callback(function()
                for _, setting in pairs(group.settings) do
                    setSettingValue(global, group.key, setting.key, setting.default)
                end
            end),
        },
        content = ui.content {
            {
                template = I.MWUI.templates.padding,
                content = ui.content {
                    {
                        template = I.MWUI.templates.textNormal,
                        props = {
                            text = interfaceL10n('Reset')
                        },
                    },
                },
            },
        },
    }

    local titleLayout = {
        type = ui.TYPE.Flex,
        external = {
            stretch = 1,
        },
        content = ui.content {
            {
                template = I.MWUI.templates.textHeader,
                props = {
                    text = l10n(group.name),
                    textSize = 20,
                },
            }
        },
    }
    if group.description then
        titleLayout.content:add(interval)
        titleLayout.content:add {
            template = I.MWUI.templates.textParagraph,
            props = {
                text = l10n(group.description),
                size = util.vector2(300, 0),
            },
        }
    end

    return {
        name = groupLayoutName(group.key, global),
        type = ui.TYPE.Flex,
        external = {
            stretch = 1,
        },
        content = ui.content {
            {
                type = ui.TYPE.Flex,
                props = {
                    horizontal = true,
                    arrange = ui.ALIGNMENT.Center,
                },
                external = {
                    stretch = 1,
                },
                content = ui.content {
                    titleLayout,
                    growingIntreval,
                    resetButtonLayout,
                },
            },
            spacedLines(2),
            {
                name = 'settings',
                type = ui.TYPE.Flex,
                content = settingsContent,
                external = {
                    stretch = 1,
                },
            },
        },
    }
end

local function pageGroupComparator(a, b)
    return a.order < b.order or (
        a.order == b.order and a.key < b.key
    )
end

local function generateSearchHints(page)
    local hints = {}
    do
        local l10n = core.l10n(page.l10n)
        table.insert(hints, l10n(page.name))
        if page.description then
            table.insert(hints, l10n(page.description))
        end
    end
    local pageGroups = groups[page.key]
    for _, pageGroup in pairs(pageGroups) do
        local group = common.getSection(pageGroup.global, common.groupSectionKey):get(pageGroup.key)
        local l10n = core.l10n(group.l10n)
        table.insert(hints, l10n(group.name))
        if group.description then
            table.insert(hints, l10n(group.description))
        end
        for _, setting in pairs(group.settings) do
            table.insert(hints, l10n(setting.name))
            if setting.description then
                table.insert(hints, l10n(setting.description))
            end
        end
    end
    return table.concat(hints, ' ')
end

local function renderPage(page, options)
    local l10n = core.l10n(page.l10n)
    local sortedGroups = {}
    for _, group in pairs(groups[page.key]) do
        table.insert(sortedGroups, group)
    end
    table.sort(sortedGroups, pageGroupComparator)
    local groupLayouts = {}
    for _, pageGroup in ipairs(sortedGroups) do
        local group = common.getSection(pageGroup.global, common.groupSectionKey):get(pageGroup.key)
        if not group then
            error(string.format('%s group "%s" was not found', pageGroup.global and 'Global' or 'Player', pageGroup.key))
        end
        local groupElement = groupElements[page.key][group.key]
        if not groupElement or not groupElement.layout then
            groupElement = ui.create(renderGroup(group, pageGroup.global))
        end
        if groupElement.layout == nil then
            error(string.format('Destroyed group element for %s %s', page.key, group.key))
        end
        groupElements[page.key][group.key] = groupElement
        table.insert(groupLayouts, groupElement)
    end
    local groupsLayout = {
        name = 'groups',
        type = ui.TYPE.Flex,
        external = {
            stretch = 1,
        },
        content = ui.content(interlaceSeparator(groupLayouts, bigSpacer)),
    }
    local titleLayout = {
        type = ui.TYPE.Flex,
        external = {
            stretch = 1,
        },
        content = ui.content {
            {
                template = I.MWUI.templates.textHeader,
                props = {
                    text = l10n(page.name),
                    textSize = 22,
                },
            },
            spacedLines(3),
        },
    }
    if page.description then
        titleLayout.content:add {
            template = I.MWUI.templates.textParagraph,
            props = {
                text = l10n(page.description),
                size = util.vector2(300, 0),
            },
        }
    end
    local layout = {
        name = page.key,
        type = ui.TYPE.Flex,
        props = {
            position = util.vector2(10, 10),
        },
        content = ui.content {
            titleLayout,
            bigSpacer,
            groupsLayout,
            bigSpacer,
        },
    }
    options.name = l10n(page.name)
    options.searchHints = generateSearchHints(page)
    if options.element then
        options.element.layout = layout
        options.element:update()
    else
        options.element = ui.create(layout)
    end
end

local function onSettingChanged(global)
    return async:callback(function(groupKey, settingKey)
        local group = common.getSection(global, common.groupSectionKey):get(groupKey)
        if not group or not pageOptions[group.page] then return end

        local groupElement = groupElements[group.page][group.key]

        if not settingKey then
            if groupElement then
                groupElement.layout = renderGroup(group)
                groupElement:update()
            else
                renderPage(pages[group.page], pageOptions[group.page])
            end
            return
        end

        local value = common.getSection(global, group.key):get(settingKey)
        local settingsContent = groupElement.layout.content.settings.content
        auxUi.deepDestroy(settingsContent[settingKey]) -- support setting renderers which return UI elements
        settingsContent[settingKey] = renderSetting(group, group.settings[settingKey], value, global)
        groupElement:update()
    end)
end

local function onGroupRegistered(global, key)
    local group = common.getSection(global, common.groupSectionKey):get(key)
    if not group then return end

    groups[group.page] = groups[group.page] or {}
    groupElements[group.page] = groupElements[group.page] or {}

    local pageGroup = {
        key = group.key,
        global = global,
        order = group.order,
    }

    if not groups[group.page][pageGroup.key] then
        common.getSection(global, group.key):subscribe(onSettingChanged(global))
        common.getArgumentSection(global, group.key):subscribe(async:callback(function(_, settingKey)
            if settingKey == nil then return end

            local group = common.getSection(global, common.groupSectionKey):get(group.key)
            if not group or not pageOptions[group.page] then return end

            local value = common.getSection(global, group.key):get(settingKey)

            local element = groupElements[group.page][group.key]
            local settingsContent = element.layout.content.settings.content
            settingsContent[settingKey] = renderSetting(group, group.settings[settingKey], value, global)
            element:update()
        end))
    end

    groups[group.page][pageGroup.key] = pageGroup

    if not pages[group.page] then return end
    pageOptions[group.page] = pageOptions[group.page] or {}
    renderPage(pages[group.page], pageOptions[group.page])
end

local function updateGroups(global)
    local groupSection = common.getSection(global, common.groupSectionKey)
    for groupKey in pairs(groupSection:asTable()) do
        onGroupRegistered(global, groupKey)
    end
    groupSection:subscribe(async:callback(function(_, key)
        if key then
            onGroupRegistered(global, key)
        else
            for groupKey in pairs(groupSection:asTable()) do
                onGroupRegistered(global, groupKey)
            end
        end
    end))
end
local updatePlayerGroups = function() updateGroups(false) end
local updateGlobalGroups = function() updateGroups(true) end

local menuGroups = {}
local menuPages = {}

local function resetPlayerGroups()
    local playerGroupsSection = storage.playerSection(common.groupSectionKey)
    for pageKey, page in pairs(groups) do
        for groupKey in pairs(page) do
            if not menuGroups[groupKey] then
                if groupElements[pageKey][groupKey] then
                    groupElements[pageKey][groupKey]:destroy()
                    groupElements[pageKey][groupKey] = nil
                end
                page[groupKey] = nil
                playerGroupsSection:set(groupKey, nil)
            end
        end
        local options = pageOptions[pageKey]
        if options then
            if not menuPages[pageKey] then
                if options.element then
                    auxUi.deepDestroy(options.element)
                    options.element = nil
                end
                ui.removeSettingsPage(options)
                pageOptions[pageKey] = nil
            else
                renderPage(pages[pageKey], options)
            end
        end
    end
end

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
    if options.description ~= nil and type(options.description) ~= 'string' then
        error('Page description key must be a string')
    end
    local page = {
        key = options.key,
        l10n = options.l10n,
        name = options.name,
        description = options.description,
    }
    pages[page.key] = page
    groups[page.key] = groups[page.key] or {}
    pageOptions[page.key] = pageOptions[page.key] or {}
    renderPage(page, pageOptions[page.key])
    ui.registerSettingsPage(pageOptions[page.key])
end

updatePlayerGroups()
if menu.getState() == menu.STATE.Running then -- handle reloadlua correctly
    updateGlobalGroups()
end

return {
    interfaceName = 'Settings',
    interface = {
        version = 1,
        registerPage = function(options)
            registerPage(options)
            menuPages[options.key] = true
        end,
        registerRenderer = registerRenderer,
        registerGroup = function(options)
            if not options.permanentStorage then
                error('Menu scripts are only allowed to register setting groups with permanentStorage = true')
            end
            common.registerGroup(options)
            menuGroups[options.key] = true
        end,
        updateRendererArgument = common.updateRendererArgument,
    },
    engineHandlers = {
        onStateChanged = function()
            if menu.getState() == menu.STATE.Running then
                updatePlayerGroups()
                updateGlobalGroups()
            elseif menu.getState() == menu.STATE.NoGame then
                resetPlayerGroups()
            end
        end,
    },
    eventHandlers = {
        [common.registerPageEvent] = function(options)
            registerPage(options)
        end,
    }
}
