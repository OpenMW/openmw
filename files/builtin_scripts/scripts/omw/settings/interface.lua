return function(player)
    local core = require('openmw.core')
    local types = require('openmw.types')
    local world = not player and require('openmw.world')
    

    local sections = require('scripts.omw.settings.sections')
    local render = player and require('scripts.omw.settings.render') or nil

    local settingChangeEvent = 'omwSettingChanged'
    local globalSetEvent = 'omwSettingGlobalSet'
    local registerEvent = 'omwSettingGroupRegistered'

    local groups, scopes, SCOPE, isGlobal = sections.groups, sections.scopes, sections.SCOPE, sections.isGlobal



    local saveScope = scopes[player and SCOPE.SavePlayer or SCOPE.SaveGlobal]
    return {
        interfaceName = 'Settings',
        interface = {
            getGroup = getGroup,
            SCOPE = SCOPE,
            registerGroup = not player and require('scripts.omw.settings.register') or nil,
            registerType = player and render.registerType or nil,
        },
        engineHandlers = {
            onLoad = function(saved)
                if not player then groups:reset() end
                saveScope:reset(saved)
            end,
            onSave = function()
                return saveScope:asTable()
            end,
        },
        eventHandlers = {
            [settingChangeEvent] = function(e)
                getGroup(e.groupName):__changed(e.settingName, e.value)
            end,
            [globalSetEvent] = not player and function(e)
                local setting = getSetting(e.groupName, e.settingName)
                if isGlobal(setting.scope) then
                    setSettingValue(e.groupName, e.settingName, e.value)
                else
                    error(('Unexpected Setting event for a non-global setting %s'):format(e.settingName))
                end
            end or nil,
            [registerEvent] = player and render.onGroupRegistered or nil,
        }
    }
end