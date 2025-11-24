local ambient = require('openmw.ambient')
local core = require('openmw.core')
local Skill = core.stats.Skill
local I = require('openmw.interfaces')
local self = require('openmw.self')
local types = require('openmw.types')
local NPC = types.NPC
local Actor = types.Actor
local ui = require('openmw.ui')
local auxUtil = require('openmw_aux.util')

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

local function skillLevelUpHandler(skillid, source, params)
    local skillStat = NPC.stats.skills[skillid](self)
    if (skillStat.base >= 100 and params.skillIncreaseValue > 0) or
        (skillStat.base <= 0 and params.skillIncreaseValue < 0) then
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

    if source ~= 'jail' then
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
end

local function jailTimeServed(days)
    if not days or days <= 0 then
        return
    end

    local oldSkillLevels = {}
    local skillByNumber = {}
    for skillid, skillStat in pairs(NPC.stats.skills) do
        oldSkillLevels[skillid] = skillStat(self).base
        skillByNumber[#skillByNumber+1] = skillid
    end

    math.randomseed(core.getSimulationTime())
    for day=1,days do
        local skillid = skillByNumber[math.random(#skillByNumber)]
        -- skillLevelUp() handles skill-based increase/decrease
        I.SkillProgression.skillLevelUp(skillid, I.SkillProgression.SKILL_INCREASE_SOURCES.Jail)
    end

    local message = ''
    if days == 1 then
        message = string.format(core.getGMST('sNotifyMessage42'), days)
    else
        message = string.format(core.getGMST('sNotifyMessage43'), days)
    end
    for skillid, skillStat in pairs(NPC.stats.skills) do
        local diff = skillStat(self).base - oldSkillLevels[skillid]
        if diff ~= 0 then
            local skillMsg = core.getGMST('sNotifyMessage39')
            if diff < 0 then
                skillMsg = core.getGMST('sNotifyMessage44')
            end
            local skillRecord = Skill.record(skillid)
            message = message..'\n'..string.format(skillMsg, skillRecord.name, skillStat(self).base)
        end
    end

    I.UI.showInteractiveMessage(message)
end

local function skillUsedHandler(skillid, params)
    if NPC.isWerewolf(self) then
        return false
    end

    local skillStat = NPC.stats.skills[skillid](self)

    if (skillStat.base >= 100 and params.skillGain > 0) or
        (skillStat.base <= 0 and params.skillGain < 0) then
        return false
    end

    skillStat.progress = skillStat.progress + params.skillGain / I.SkillProgression.getSkillProgressRequirement(skillid)

    if skillStat.progress >= 1 then
        I.SkillProgression.skillLevelUp(skillid, I.SkillProgression.SKILL_INCREASE_SOURCES.Usage)
    end
end

I.SkillProgression.addSkillUsedHandler(skillUsedHandler)
I.SkillProgression.addSkillLevelUpHandler(skillLevelUpHandler)

local interface = auxUtil.shallowCopy(I.SkillProgression)
interface.getSkillProgressRequirement = getSkillProgressRequirement
interface.getSkillLevelUpOptions = getSkillLevelUpOptions

return {
    engineHandlers = {
        _onJailTimeServed = jailTimeServed,
    },
    interfaceName = 'SkillProgression',
    interface = interface
}
