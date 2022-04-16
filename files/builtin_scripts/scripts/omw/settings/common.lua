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

local function getSetting(groupKey, settingKey)
    local group = groups:get(groupKey)
    if not group then
        error('Unknown group')
    end
    local setting = group.settings[settingKey]
    if not setting then
        error('Unknown setting')
    end
    return setting
end

local function getSettingValue(groupKey, settingKey)
    local setting = getSetting(groupKey, settingKey)
    local scopeSection = scopes[setting.scope]
    if not scopeSection then
        error(('Setting %s is not available in this context'):format(setting.key))
    end
    if not scopeSection:get(groupKey) then
        scopeSection:set(groupKey, {})
    end
    return scopeSection:get(groupKey)[setting.key] or setting.default
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

local function setSettingValue(groupKey, settingKey, value)
    local setting = getSetting(groupKey, settingKey)
    local event = {
        groupName = groupKey,
        settingName = setting.key,
        value = value,
    }
    if isPlayerScript and isGlobalScope(setting.scope) then
        core.sendGlobalEvent(EVENTS.SettingSet, event)
        return
    end

    local scopeSection = scopes[setting.scope]
    if not scopeSection:get(groupKey) then
        scopeSection:set(groupKey, {})
    end
    local copy = scopeSection:getCopy(groupKey)
    copy[setting.key] = value
    scopeSection:set(groupKey, copy)

    notifySettingChange(setting.scope, event)
end

local groupMeta = {
    __index = {
        get = function(self, settingKey)
            return getSettingValue(self.key, settingKey)
        end,
        set = function(self, settingKey, value)
            setSettingValue(self.key, settingKey, value)
        end,
        onChange = function(self, callback)
            table.insert(self.__callbacks, callback)
        end,
        __changed = function(self, settingKey, value)
            for _, callback in ipairs(self.__callbacks) do
                callback(settingKey, value)
            end
        end,
    },
}
local cachedGroups = {}
local function getGroup(groupKey)
    if not cachedGroups[groupKey] then
        cachedGroups[groupKey] = setmetatable({
            key = groupKey,
            __callbacks = {},
        }, groupMeta)
    end
    return cachedGroups[groupKey]
end

return {
    EVENTS = EVENTS,
    SCOPE = SCOPE,
    scopes = scopes,
    groups = groups,
    getGroup = getGroup,
}