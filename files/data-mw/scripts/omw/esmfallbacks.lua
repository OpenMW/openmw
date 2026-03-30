local content = require('openmw.content')

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

return {
    engineHandlers = {
        onContentFilesLoaded = function()
            generateDefaultDoors()
            generateDefaultGMSTs()
            generateDefaultStatics()
        end
    }
}
