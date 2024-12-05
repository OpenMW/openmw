local storage = require('openmw.storage')

local groupSectionKey = 'OmwSettingGroups'
local conventionPrefix = 'Settings'
local argumentSectionPostfix = 'Arguments'

local contextSection = storage.playerSection or storage.globalSection
local groupSection = contextSection(groupSectionKey)

local function validateSettingOptions(options)
    if type(options) ~= 'table' then
        error('Setting options must be a table')
    end
    if type(options.key) ~= 'string' then
        error('Setting must have a key')
    end
    if type(options.renderer) ~= 'string' then
        error('Setting must have a renderer')
    end
    if type(options.name) ~= 'string' then
        error('Setting must have a name localization key')
    end
    if options.description ~= nil and type(options.description) ~= 'string' then
        error('Setting description key must be a string')
    end
end

local function validateGroupOptions(options)
    if type(options) ~= 'table' then
        error('Group options must be a table')
    end
    if type(options.key) ~= 'string' then
        error('Group must have a key')
    end
    if options.key:sub(1, conventionPrefix:len()) ~= conventionPrefix then
        print(("Group key %s doesn't start with %s"):format(options.key, conventionPrefix))
    end
    if type(options.page) ~= 'string' then
        error('Group must belong to a page')
    end
    if type(options.order) ~= 'number' and type(options.order) ~= 'nil' then
        error('Group order must be a number')
    end
    if type(options.l10n) ~= 'string' then
        error('Group must have a localization context')
    end
    if type(options.name) ~= 'string' then
        error('Group must have a name localization key')
    end
    if options.description ~= nil and type(options.description) ~= 'string' then
        error('Group description key must be a string')
    end
    if type(options.permanentStorage) ~= 'boolean' then
        error('Group must have a permanentStorage flag')
    end
    if type(options.settings) ~= 'table' then
        error('Group must have a table of settings')
    end
    for _, opt in ipairs(options.settings) do
        validateSettingOptions(opt)
    end
end

local function registerSetting(options)
    return {
        key = options.key,
        default = options.default,
        renderer = options.renderer,
        argument = options.argument,

        name = options.name,
        description = options.description,
    }
end

local function registerGroup(options)
    validateGroupOptions(options)
    if groupSection:get(options.key) then
        error(('Group with key %s was already registered'):format(options.key))
    end
    local group = {
        key = options.key,
        page = options.page,
        order = options.order or 0,
        l10n = options.l10n,
        name = options.name,
        description = options.description,
        permanentStorage = options.permanentStorage,
        settings = {},
    }
    local valueSection = contextSection(options.key)
    local argumentSection = contextSection(options.key .. argumentSectionPostfix)
    for i, opt in ipairs(options.settings) do
        local setting = registerSetting(opt)
        setting.order = i
        if group.settings[setting.key] then
            error(('Duplicate setting key %s'):format(options.key))
        end
        group.settings[setting.key] = setting
        if valueSection:get(setting.key) == nil then
            valueSection:set(setting.key, setting.default)
        end
        argumentSection:set(setting.key, setting.argument)
    end
    groupSection:set(group.key, group)
end

return {
    getSection = function(global, key)
        return (global and storage.globalSection or storage.playerSection)(key)
    end,
    getArgumentSection = function(global, key)
        return (global and storage.globalSection or storage.playerSection)(key .. argumentSectionPostfix)
    end,
    updateRendererArgument = function(groupKey, settingKey, argument)
        local argumentSection = contextSection(groupKey .. argumentSectionPostfix)
        argumentSection:set(settingKey, argument)
    end,
    setGlobalEvent = 'OMWSettingsGlobalSet',
    registerPageEvent = 'OmWSettingsRegisterPage',
    groupSectionKey = groupSectionKey,
    onLoad = function(saved)
        if not saved then return end
        for groupKey, settings in pairs(saved) do
            local section = contextSection(groupKey)
            for key, value in pairs(settings) do
                section:set(key, value)
            end
        end
    end,
    onSave = function()
        local saved = {}
        for groupKey, group in pairs(groupSection:asTable()) do
            if not group.permanentStorage then
                saved[groupKey] = contextSection(groupKey):asTable()
            end
        end
        return saved
    end,
    registerGroup = registerGroup,
}
