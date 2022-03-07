local prequire = function(path)
    local status, result = pcall(function()
        return require(path)
    end)
    return status and result or nil
end

local core = require('openmw.core')
local types = require('openmw.types')
local storage = require('openmw.storage')
local self = prequire('openmw.self')
local world = prequire('openmw.world')

local isPlayerScript = self and true or false
local isGlobalScript = world and true or false

local EVENTS = {
    SettingChanged = 'omwSettingsChanged',
    SettingSet = 'omwSettingsGlobalSet',
    GroupRegistered = 'omwSettingsGroupRegistered',
}

local SCOPE = {
    Global = 'Global',
    Player = 'Player',
    SaveGlobal = 'SaveGlobal',
    SavePlayer = 'SavePlayer',
}

local groups = storage.globalSection('OMW_Settings_Groups')
local saveGlobalSection = storage.globalSection('OMW_Settings_SaveGlobal')

if isGlobalScript then
    groups:removeOnExit()
    saveGlobalSection:removeOnExit()
end

local savePlayerSection = nil
if isPlayerScript then
    savePlayerSection = storage.playerSection('OMW_Setting_SavePlayer')
    savePlayerSection:removeOnExit()
end

local scopes = {
    [SCOPE.Global] = storage.globalSection('OMW_Setting_Global'),
    [SCOPE.Player] = isPlayerScript and storage.playerSection('OMW_Setting_Player'),
    [SCOPE.SaveGlobal] = saveGlobalSection,
    [SCOPE.SavePlayer] = savePlayerSection,
}

local function isGlobalScope(scope)
    return scope == SCOPE.Global or scope == SCOPE.SaveGlobal
end

local function getSetting(groupName, settingName)
    local group = groups:get(groupName)
    if not group then
        error('Unknown group')
    end
    local setting = group[settingName]
    if not setting then
        error('Unknown setting')
    end
    return setting
end

local function getSettingValue(groupName, settingName)
    local setting = getSetting(groupName, settingName)
    local scopeSection = scopes[setting.scope]
    if not scopeSection then
        error(('Setting %s is not available in this context'):format(setting.name))
    end
    if not scopeSection:get(groupName) then
        scopeSection:set(groupName, {})
    end
    return scopeSection:get(groupName)[setting.name] or setting.default
end

local function notifySettingChange(scope, event)
    if isGlobalScope(scope) then
        core.sendGlobalEvent(EVENTS.SettingChanged, event)
        for _, a in ipairs(world.activeActors) do
            if a.type == types.Player then
                a:sendEvent(EVENTS.SettingChanged, event)
            end
        end
    else
        self:sendEvent(EVENTS.SettingChanged, event)
    end
end

local function setSettingValue(groupName, settingName, value)
    local setting = getSetting(groupName, settingName)
    local event = {
        groupName = groupName,
        settingName = setting.name,
        value = value,
    }
    if isPlayerScript and isGlobalScope(setting.scope) then
        core.sendGlobalEvent(EVENTS.SettingSet, event)
        return
    end

    local scopeSection = scopes[setting.scope]
    if not scopeSection:get(groupName) then
        scopeSection:set(groupName, {})
    end
    local copy = scopeSection:getCopy(groupName)
    copy[setting.name] = value
    scopeSection:set(groupName, copy)

    notifySettingChange(setting.scope, event)
end

local groupMeta = {
    __index = {
        get = function(self, settingName)
            return getSettingValue(self.name, settingName)
        end,
        set = function(self, settingName, value)
            setSettingValue(self.name, settingName, value)
        end,
        onChange = function(self, callback)
            table.insert(self.__callbacks, callback)
        end,
        __changed = function(self, settingName, value)
            for _, callback in ipairs(self.__callbacks) do
                callback(settingName, value)
            end
        end,
    },
}
local cachedGroups = {}
local function getGroup(groupName)
    if not cachedGroups[groupName] then
        cachedGroups[groupName] = setmetatable({
            name = groupName,
            __callbacks = {},
        }, groupMeta)
    end
    return cachedGroups[groupName]
end

return {
    EVENTS = EVENTS,
    SCOPE = SCOPE,
    scopes = scopes,
    groups = groups,
    getGroup = getGroup,
}