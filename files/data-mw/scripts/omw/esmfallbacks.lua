local content = require('openmw.content')

local function generateAttributes()
    local gmsts = content.gameSettings.records
    local attributes = content.attributes.records
    local function getString(key)
        local v = gmsts[key]
        if type(v) ~= 'string' or v == '' then
            return key
        end
        return v
    end

    local values = {
        { id = 'Strength', name = 'sAttributeStrength', description = 'sStrDesc', icon = 'icons/k/attribute_strength.dds', werewolf = 'fWerewolfStrength' },
        { id = 'Intelligence', name = 'sAttributeIntelligence', description = 'sIntDesc', icon = 'icons/k/attribute_int.dds', werewolf = 'fWerewolfIntellegence' },
        { id = 'Willpower', name = 'sAttributeWillpower', description = 'sWilDesc', icon = 'icons/k/attribute_wilpower.dds', werewolf = 'fWerewolfWillpower' },
        { id = 'Agility', name = 'sAttributeAgility', description = 'sAgiDesc', icon = 'icons/k/attribute_agility.dds', werewolf = 'fWerewolfAgility' },
        { id = 'Speed', name = 'sAttributeSpeed', description = 'sSpdDesc', icon = 'icons/k/attribute_speed.dds', werewolf = 'fWerewolfSpeed' },
        { id = 'Endurance', name = 'sAttributeEndurance', description = 'sEndDesc', icon = 'icons/k/attribute_endurance.dds', werewolf = 'fWerewolfEndurance' },
        { id = 'Personality', name = 'sAttributePersonality', description = 'sPerDesc', icon = 'icons/k/attribute_personality.dds', werewolf = 'fWerewolfPersonality' },
        { id = 'Luck', name = 'sAttributeLuck', description = 'sLucDesc', icon = 'icons/k/attribute_luck.dds', werewolf = 'fWerewolfLuck' },
    }
    for _, value in pairs(values) do
        local werewolfValue = gmsts[value.werewolf]
        if type(werewolfValue) ~= 'number' then
            werewolfValue = 0
        end
        attributes[value.id] = {
            name = getString(value.name),
            description = getString(value.description),
            icon = value.icon,
            werewolfValue = werewolfValue,
        }
    end
end

local function generateDefaultStatics()
    local statics = {
        -- Total conversions from SureAI lack marker records
        divinemarker = 'meshes/marker_divine.nif',
        doormarker = 'meshes/marker_arrow.nif',
        northmarker = 'meshes/marker_north.nif',
        templemarker = 'meshes/marker_temple.nif',
        travelmarker = 'meshes/marker_travel.nif',
    }
    for id, model in pairs(statics) do
        if content.statics.records[id] == nil then
            content.statics.records[id] = { model = model }
        end
    end
end

local function generateDefaultGMSTs()
    local gmsts = {
        -- Companion (tribunal)
        sCompanionShare = 'Companion Share',
        sCompanionWarningMessage = 'Warning message',
        sCompanionWarningButtonOne = 'Button 1',
        sCompanionWarningButtonTwo = 'Button 2',
        sProfitValue = 'Profit Value',
        sTeleportDisabled = 'Teleport disabled',
        sLevitateDisabled = 'Levitate disabled',
        -- Missing in unpatched MW 1.0
        sDifficulty = 'Difficulty',
        fDifficultyMult = 5,
        sAuto_Run = 'Auto Run',
        sServiceRefusal = 'Service Refusal',
        sNeedOneSkill = 'Need one skill',
        sNeedTwoSkills = 'Need two skills',
        sEasy = 'Easy',
        sHard = 'Hard',
        sDeleteNote = 'Delete Note',
        sEditNote = 'Edit Note',
        sAdmireSuccess = 'Admire Success',
        sAdmireFail = 'Admire Fail',
        sIntimidateSuccess = 'Intimidate Success',
        sIntimidateFail = 'Intimidate Fail',
        sTauntSuccess = 'Taunt Success',
        sTauntFail = 'Taunt Fail',
        sBribeSuccess = 'Bribe Success',
        sBribeFail = 'Bribe Fail',
        fNPCHealthBarTime = 5,
        fNPCHealthBarFade = 1,
        fFleeDistance = 3000,
        sMaxSale = 'Max Sale',
        sAnd = 'and',
        -- Werewolf (BM)
        fWereWolfRunMult = 1.3,
        fWereWolfSilverWeaponDamageMult = 2,
        iWerewolfFightMod = 100,
        iWereWolfFleeMod = 100,
        iWereWolfLevelToAttack = 20,
        iWereWolfBounty = 1000,
        fCombatDistanceWerewolfMod = 0.3,
    }
    local store = content.gameSettings.records
    for id, value in pairs(gmsts) do
        if store[id] == nil then
            store[id] = value
        end
    end
    local fallbacks = content.gameSettings.getFallbacks()
    for id, value in pairs(fallbacks) do
        store[id] = value
    end
end

local function generateDefaultDoors()
    local doors = {
        prisonmarker = 'meshes/marker_prison.nif'
    }
    for id, model in pairs(doors) do
        if content.doors.records[id] == nil then
            content.doors.records[id] = { model = model }
        end
    end
end

local function setMagicEffectNames()
    local gmsts = content.gameSettings.records
    local effects = content.magicEffects.records
    for id, gmst in pairs(content.magicEffects._getGMSTs()) do
        local effect = effects[id]
        if effect ~= nil then
            local name = gmsts[gmst]
            if type(name) ~= 'string' or name == '' then
                name = gmst
            end
            effect.name = name
        end
    end
end

return {
    engineHandlers = {
        onContentFilesLoaded = function()
            generateAttributes()
            generateDefaultDoors()
            generateDefaultGMSTs()
            generateDefaultStatics()
            setMagicEffectNames()
        end
    }
}
