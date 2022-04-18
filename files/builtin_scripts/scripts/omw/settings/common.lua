local storage = require('openmw.storage')
local core = require('openmw.core')
local types = require('openmw.types')
local selfObject
do
    local success, result = pcall(function() return require('openmw.self') end)
    selfObject = success and result or nil
end
local playerObject = selfObject and selfObject.type == types.Player and selfObject or nil

local eventPrefix = 'omwSettings'
local EVENTS = {
    SettingChanged = eventPrefix .. 'Changed',
    SetValue = eventPrefix .. 'GlobalSetValue',
    GroupRegistered = eventPrefix .. 'GroupRegistered',
    RegisterGroup = eventPrefix .. 'RegisterGroup',
    Subscribe = eventPrefix .. 'Subscribe',
}

local SCOPE = {
    Global = 'Global',
    Player = 'Player',
    SaveGlobal = 'SaveGlobal',
    SavePlayer = 'SavePlayer',
}

local function isPlayerScope(scope)
    return scope == SCOPE.Player or scope == SCOPE.SavePlayer
end

local function isSaveScope(scope)
    return scope == SCOPE.SaveGlobal or scope == SCOPE.SavePlayer
end

local prefix = 'omw_settings_'
local settingsPattern = prefix .. 'settings_%s%s'

local groupsSection = storage.globalSection(prefix .. 'groups')
if groupsSection.removeOnExit then
    groupsSection:removeOnExit()
end

local function values(groupKey, scope)
    local player = isPlayerScope(scope)
    local save = isSaveScope(scope)
    local sectionKey = settingsPattern:format(groupKey, save and '_save' or '')
    local section
    if player then
        section = storage.playerSection and storage.playerSection(sectionKey) or nil
    else
        section = storage.globalSection(sectionKey)
    end
    if save and section and section.removeOnExit then
        section:removeOnExit()
    end
    return section
end

local function saveScope(scope)
    local saved = {}
    for _, group in pairs(groupsSection:asTable()) do
        saved[group.key] = values(group.key, scope):asTable()
    end
    return saved
end

local function loadScope(scope, saved)
    if not saved then return end
    for _, group in pairs(saved) do
        values(group.key, scope):reset(saved[group.key])
    end
end

local function groupSubscribeEvent(groupKey)
    return ('%sSubscribe%s'):format(eventPrefix, groupKey)
end

local subscriptions = {}
local function handleSubscription(event)
    if not subscriptions[event.groupKey] then
        subscriptions[event.groupKey] = {}
    end
    table.insert(subscriptions[event.groupKey], event.object or false)
end

local function subscribe(self)
    local groupKey = rawget(self, 'groupKey')
    local event = {
        groupKey = groupKey,
        object = selfObject,
    }
    core.sendGlobalEvent(EVENTS.Subscribe, event)
    if playerObject then
        playerObject:sendEvent(EVENTS.Subscribe, event)
    end
    return groupSubscribeEvent(groupKey)
end

local groupMeta = {
    __newindex = function(self, settingKey, value)
        local group = groupsSection:get(rawget(self, 'groupKey'))
        local setting = group.settings[settingKey]
        if not setting then
            error(('Setting %s does not exist'):format(settingKey))
        end
        local section = values(group.key, setting.scope)
        local event = {
            groupKey = group.key,
            settingKey = settingKey,
            value = value,
        }
        if section.set then
            section:set(settingKey, value)
            if playerObject then
                playerObject:sendEvent(EVENTS.SettingChanged, event)
            else
                core.sendGlobalEvent(EVENTS.SettingChanged, event)
            end
            if subscriptions[group.key] then
                local eventKey = groupSubscribeEvent(group.key)
                for _, object in ipairs(subscriptions[group.key]) do
                    if object then
                        object:sendEvent(eventKey, event)
                    else
                        core.sendGlobalEvent(eventKey, event)
                    end
                end
            end
        else
            if isPlayerScope(setting.scope) then
                error(("Can't change player scope setting %s from global scope"):format(settingKey))
            else
                core.sendGlobalEvent(EVENTS.SetValue, event)
            end
        end
    end,
    __index = function(self, key)
        if key == "subscribe" then return subscribe end
        local settingKey = key
        local group = groupsSection:get(rawget(self, 'groupKey'))
        local setting = group.settings[settingKey]
        if not setting then
            error(('Unknown setting %s'):format(settingKey))
        end
        local section = rawget(self, 'sections')[setting.scope]
        if not section then
            error(("Can't access setting %s from scope %s"):format(settingKey, setting.scope))
        end
        return section:get(setting.key) or setting.default
    end,
}

local function group(groupKey)
    if not groupsSection:get(groupKey) then
        print(("Settings group %s wasn't registered yet"):format(groupKey))
    end
    local s = {}
    for _, scope in pairs(SCOPE) do
        local section = values(groupKey, scope)
        if section then
            s[scope] = section
        end
    end
    return setmetatable({
        groupKey = groupKey,
        sections = s,
    }, groupMeta)
end

return {
    SCOPE = SCOPE,
    EVENTS = EVENTS,
    isPlayerScope = isPlayerScope,
    isSaveScope = isSaveScope,
    values = values,
    groups = function()
        return groupsSection
    end,
    saveScope = saveScope,
    loadScope = loadScope,
    group = group,
    handleSubscription = handleSubscription,
}