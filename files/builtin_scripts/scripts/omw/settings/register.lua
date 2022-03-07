local world = require('openmw.world')
local types = require('openmw.types')

local common = require('scripts.omw.settings.common')

local groups, SCOPE = common.groups, common.SCOPE

local function validScope(scope)
    local valid = false
    for _, v in pairs(SCOPE) do
        if v == scope then
            valid = true
            break
        end
    end
    return valid
end

local function validateSettingOptions(options)
    if type(options.name) ~= 'string' then
        error('Setting must have a name')
    end
    if options.default == nil then
        error('Setting must have a default value')
    end
    if type(options.description) ~= 'string' then
        error('Setting must have a description')
    end
    if not validScope(options.scope) then
        error(('Invalid setting scope %s'):format(options.scope))
    end
    if type(options.renderer) ~= 'string' then
        error('Setting must have a renderer')
    end
end

local function addSetting(group, options)
    validateSettingOptions(options)
    if group[options.name] then
        error(('Duplicate setting name %s'):format(options.name))
    end
    group[options.name] = {
        name = options.name,
        scope = options.scope or SCOPE.Global,
        default = options.default,
        description = options.description,
        renderer = options.renderer,
    }
end

local function registerGroup(groupName, list)
    if groups:get(groupName) then
        print(('Overwriting group %s'):format(groupName))
    end
    local settings = {}
    for _, opt in ipairs(list) do
        addSetting(settings, opt)
    end
    groups:set(groupName, settings)
    for _, a in ipairs(world.activeActors) do
        if a.type == types.Player and a:isValid() then
            a:sendEvent(common.EVENTS.GroupRegistered, groupName)
        end
    end
end

local function onPlayerAdded(player)
    for groupName in pairs(groups:asTable()) do
        player:sendEvent(common.EVENTS.GroupRegistered, groupName)
    end
end

return {
    registerGroup = registerGroup,
    onPlayerAdded = onPlayerAdded,
}