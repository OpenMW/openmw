local world = require('openmw.world')
local types = require('openmw.types')

local common = require('scripts.omw.settings.common')

local function validScope(scope)
    local valid = false
    for _, v in pairs(common.SCOPE) do
        if v == scope then
            valid = true
            break
        end
    end
    return valid
end

local function validateSettingOptions(options)
    if type(options.key) ~= 'string' then
        error('Setting must have a key')
    end
    if not validScope(options.scope) then
        error(('Invalid setting scope %s'):format(options.scope))
    end
    if type(options.renderer) ~= 'string' then
        error('Setting must have a renderer')
    end
    if type(options.name) ~= 'string' then
        error('Setting must have a name localization key')
    end
    if type(options.description) ~= 'string' then
        error('Setting must have a descripiton localization key')
    end
end

local function addSetting(settings, options)
    validateSettingOptions(options)
    if settings[options.key] then
        error(('Duplicate setting key %s'):format(options.key))
    end
    settings[options.key] = {
        key = options.key,
        scope = options.scope,
        default = options.default,
        renderer = options.renderer,
        argument = options.argument,

        name = options.name,
        description = options.description,
    }
end

local function validateGroupOptions(options)
    if type(options.key) ~= 'string' then
        error('Group must have a key')
    end
    if type(options.localization) ~= 'string' then
        error('Group must have a localization context')
    end
    if type(options.name) ~= 'string' then
        error('Group must have a name localization key')
    end
    if type(options.description) ~= 'string' then
        error('Group must have a description localization key')
    end
    if type(options.settings) ~= 'table' then
        error('Group must have a table of settings')
    end
end

local function registerGroup(options)
    local groups = common.groups()
    validateGroupOptions(options)
    if groups:get(options.key) then
        error(('Duplicate group %s'):format(options.key))
    end             
    local group = {
        key = options.key,
        localization = options.localization,
        name = options.name,
        description = options.description,

        settings = {},
    }
    for _, opt in ipairs(options.settings) do
        addSetting(group.settings, opt)
    end
    groups:set(options.key, group)
    for _, a in ipairs(world.activeActors) do
        if a.type == types.Player and a:isValid() then
            a:sendEvent(common.EVENTS.GroupRegistered, options.key)
        end
    end
end

local function onPlayerAdded(player)
    for groupName in pairs(common.groups():asTable()) do
        player:sendEvent(common.EVENTS.GroupRegistered, groupName)
    end
end

return {
    registerGroup = registerGroup,
    onPlayerAdded = onPlayerAdded,
}