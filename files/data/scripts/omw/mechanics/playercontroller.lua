local ambient = require('openmw.ambient')
local core = require('openmw.core')
local Skill = core.stats.Skill
local I = require('openmw.interfaces')
local nearby = require('openmw.nearby')
local self = require('openmw.self')
local types = require('openmw.types')
local NPC = types.NPC
local Actor = types.Actor
local ui = require('openmw.ui')

local cell = nil
local autodoors = {}

local function onCellChange()
    autodoors = {}
    for _, door in ipairs(nearby.doors) do
        if door.type == types.ESM4Door and types.ESM4Door.record(door).isAutomatic then
            autodoors[#autodoors + 1] = door
        end
    end
end

local autodoorActivationDist = 300

local lastAutoActivation = 0
local function processAutomaticDoors()
    if core.getRealTime() - lastAutoActivation < 2 then
        return
    end
    for _, door in ipairs(autodoors) do
        if door.enabled and (door.position - self.position):length() < autodoorActivationDist then
            print('Automatic activation of', door)
            door:activateBy(self)
            lastAutoActivation = core.getRealTime()
        end
    end
end

local function skillLevelUpHandler(skillid, source, params)
    local skillStat = NPC.stats.skills[skillid](self)
    if skillStat.base >= 100 then 
        return false 
    end

    if params.skillIncreaseValue then
        skillStat.base = skillStat.base + params.skillIncreaseValue
    end
    
    local levelStat = Actor.stats.level(self)
    if params.levelUpProgress then
        levelStat.progress = levelStat.progress + params.levelUpProgress
    end

    if params.levelUpAttribute and params.levelUpAttributeIncreaseValue then
        levelStat.skillIncreasesForAttribute[params.levelUpAttribute]
            = levelStat.skillIncreasesForAttribute[params.levelUpAttribute] + params.levelUpAttributeIncreaseValue
    end

    if params.levelUpSpecialization and params.levelUpSpecializationIncreaseValue then
        levelStat.skillIncreasesForSpecialization[params.levelUpSpecialization]
            = levelStat.skillIncreasesForSpecialization[params.levelUpSpecialization] + params.levelUpSpecializationIncreaseValue;
    end

    local skillRecord = Skill.record(skillid)
    local npcRecord = NPC.record(self)
    local class = NPC.classes.record(npcRecord.class)

    ambient.playSound("skillraise")

    local message = string.format(core.getGMST('sNotifyMessage39'),skillRecord.name,skillStat.base)

    if source == I.SkillProgression.SKILL_INCREASE_SOURCES.Book then
        message = '#{sBookSkillMessage}\n'..message
    end

    ui.showMessage(message, { showInDialogue = false })
    
    if levelStat.progress >= core.getGMST('iLevelUpTotal') then
        ui.showMessage('#{sLevelUpMsg}', { showInDialogue = false })
    end

    if not source or source == I.SkillProgression.SKILL_INCREASE_SOURCES.Usage then skillStat.progress = 0 end
end

local function skillUsedHandler(skillid, params)
    if NPC.isWerewolf(self) then
        return false
    end

    local skillStat = NPC.stats.skills[skillid](self)
    skillStat.progress = skillStat.progress + params.skillGain / I.SkillProgression.getSkillProgressRequirement(skillid)

    if skillStat.progress >= 1 then
        I.SkillProgression.skillLevelUp(skillid, I.SkillProgression.SKILL_INCREASE_SOURCES.Usage)
    end
end

local function onUpdate(dt)
    if dt <= 0 then
        return
    end

    if self.cell ~= cell then
        cell = self.cell
        onCellChange()
    end
    processAutomaticDoors()
end

I.SkillProgression.addSkillUsedHandler(skillUsedHandler)
I.SkillProgression.addSkillLevelUpHandler(skillLevelUpHandler)

return {
    engineHandlers = {
        onUpdate = onUpdate,
    },

    eventHandlers = {
        ShowMessage = function(data)
            if data.message then ui.showMessage(data.message) end
        end
    },
}
