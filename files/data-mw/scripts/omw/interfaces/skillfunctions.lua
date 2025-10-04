local self = require('openmw.self')
local core = require('openmw.core')
local NPC = require('openmw.types').NPC
local Skill = core.stats.Skill

local function tableHasValue(table, value)
    for _, v in pairs(table) do 
        if v == value then return true end 
    end
    return false
end

local function getSkillProgressRequirement(skillid)
    local npcRecord = NPC.record(self)
    local class = NPC.classes.record(npcRecord.class)
    local skillStat = NPC.stats.skills[skillid](self)
    local skillRecord = Skill.record(skillid)
    
    local factor = core.getGMST('fMiscSkillBonus')
    if tableHasValue(class.majorSkills, skillid) then 
        factor = core.getGMST('fMajorSkillBonus')
    elseif tableHasValue(class.minorSkills, skillid) then 
        factor = core.getGMST('fMinorSkillBonus')
    end

    if skillRecord.specialization == class.specialization then
        factor = factor * core.getGMST('fSpecialSkillBonus')
    end

    return (skillStat.base + 1) * factor
end

local function getSkillLevelUpOptions(skillid, source)
    local skillRecord = Skill.record(skillid)
    local npcRecord = NPC.record(self)
    local class = NPC.classes.record(npcRecord.class)
    
    local levelUpProgress = 0
    local levelUpAttributeIncreaseValue = core.getGMST('iLevelupMiscMultAttriubte')

    if tableHasValue(class.minorSkills, skillid) then 
        levelUpProgress = core.getGMST('iLevelUpMinorMult')
        levelUpAttributeIncreaseValue = core.getGMST('iLevelUpMinorMultAttribute')
    elseif tableHasValue(class.majorSkills, skillid) then 
        levelUpProgress = core.getGMST('iLevelUpMajorMult')
        levelUpAttributeIncreaseValue = core.getGMST('iLevelUpMajorMultAttribute')
    end

    local options = {}
    if source == 'jail' and not (skillid == 'security' or skillid == 'sneak') then
        options.skillIncreaseValue = -1
    else
        options.skillIncreaseValue = 1
        options.levelUpProgress = levelUpProgress
        options.levelUpAttribute = skillRecord.attribute
        options.levelUpAttributeIncreaseValue = levelUpAttributeIncreaseValue
        options.levelUpSpecialization = skillRecord.specialization
        options.levelUpSpecializationIncreaseValue = core.getGMST('iLevelupSpecialization')
    end
    return options
end

return {
    getSkillProgressRequirement = getSkillProgressRequirement,
    getSkillLevelUpOptions = getSkillLevelUpOptions
}
