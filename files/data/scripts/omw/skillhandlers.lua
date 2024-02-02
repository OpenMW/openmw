local self = require('openmw.self')
local I = require('openmw.interfaces')
local types = require('openmw.types')
local core = require('openmw.core')
local NPC = require('openmw.types').NPC
local Skill = core.stats.Skill

---
-- Table of skill use types defined by morrowind.
-- Each entry corresponds to an index into the available skill gain values 
-- of a @{openmw.types#SkillRecord}
-- @type SkillUseType
-- @field #number Armor_HitByOpponent 0
-- @field #number Block_Success 0
-- @field #number Spellcast_Success 0
-- @field #number Weapon_SuccessfulHit 0
-- @field #number Alchemy_CreatePotion 0
-- @field #number Alchemy_UseIngredient 1
-- @field #number Enchant_Recharge 0
-- @field #number Enchant_UseMagicItem 1
-- @field #number Enchant_CreateMagicItem 2
-- @field #number Enchant_CastOnStrike 3
-- @field #number Acrobatics_Jump 0
-- @field #number Acrobatics_Fall 1
-- @field #number Mercantile_Success 0
-- @field #number Mercantile_Bribe 1
-- @field #number Security_DisarmTrap 0
-- @field #number Security_PickLock 1
-- @field #number Sneak_AvoidNotice 0
-- @field #number Sneak_PickPocket 1
-- @field #number Speechcraft_Success 0
-- @field #number Speechcraft_Fail 1
-- @field #number Armorer_Repair 0
-- @field #number Athletics_RunOneSecond 0
-- @field #number Athletics_SwimOneSecond 0

---
-- Table of valid sources for skill increases
-- @type SkillLevelUpSource
-- @field #string Book book
-- @field #string Trainer trainer
-- @field #string Usage usage


local skillUsedHandlers = {}
local skillLevelUpHandlers = {}

local function tableHasValue(table, value)
    for _, v in pairs(table) do 
        if v == value then return true end 
    end
    return false
end

local function getSkillProgressRequirementUnorm(npc, skillid)
    local npcRecord = NPC.record(npc)
    local class = NPC.classes.record(npcRecord.class)
    local skillStat = NPC.stats.skills[skillid](npc)
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

local function skillUsed(skillid, useType, scale)
    if #skillUsedHandlers == 0 then
        -- If there are no handlers, then there won't be any effect, so skip calculations
        return
    end

    if useType > 3 or useType < 0 then
        print('Error: Unknown useType: '..tostring(useType))
        return
    end

    -- Compute skill gain
    local skillStat = NPC.stats.skills[skillid](self)
    local skillRecord = Skill.record(skillid)
    local skillGainUnorm = skillRecord.skillGain[useType + 1]
    if scale then skillGainUnorm = skillGainUnorm * scale end
    local skillProgressRequirementUnorm = getSkillProgressRequirementUnorm(self, skillid)
    local skillGain = skillGainUnorm / skillProgressRequirementUnorm

    -- Put skill gain in a table so that handlers can modify it
    local options = { 
        skillGain = skillGain,
    }

    for i = #skillUsedHandlers, 1, -1 do
        if skillUsedHandlers[i](skillid, useType, options) == false then
            return
        end
    end
end

local function skillLevelUp(skillid, source)
    if #skillLevelUpHandlers == 0 then
        -- If there are no handlers, then there won't be any effect, so skip calculations
        return
    end

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

    local options = 
    {
        skillIncreaseValue = 1,
        levelUpProgress = levelUpProgress,
        levelUpAttribute = skillRecord.attribute,
        levelUpAttributeIncreaseValue = levelUpAttributeIncreaseValue,
        levelUpSpecialization = skillRecord.specialization,
        levelUpSpecializationIncreaseValue = core.getGMST('iLevelupSpecialization'),
    }

    for i = #skillLevelUpHandlers, 1, -1 do
        if skillLevelUpHandlers[i](skillid, source, options) == false then
            return
        end
    end
end

return {
    interfaceName = 'SkillProgression',
    ---
    -- Allows to extend or override built-in skill progression mechanics.
    -- @module SkillProgression
    -- @usage local I = require('openmw.interfaces')
    --
    -- -- Forbid increasing destruction skill past 50
    -- I.SkillProgression.addSkillLevelUpHandler(function(skillid, options) 
    --     if skillid == 'destruction' and types.NPC.stats.skills.destruction(self).base >= 50 then
    --         return false
    --     end
    -- end)
    --
    -- -- Scale sneak skill progression based on active invisibility effects
    -- I.SkillProgression.addSkillUsedHandler(function(skillid, useType, params)
    --     if skillid == 'sneak' and useType == I.SkillProgression.SKILL_USE_TYPES.Sneak_AvoidNotice then
    --         local activeEffects = Actor.activeEffects(self)
    --         local visibility = activeEffects:getEffect(core.magic.EFFECT_TYPE.Chameleon).magnitude / 100
    --         visibility = visibility + activeEffects:getEffect(core.magic.EFFECT_TYPE.Invisibility).magnitude
    --         visibility = 1 - math.min(1, math.max(0, visibility))
    --         local oldSkillGain = params.skillGain
    --         params.skillGain = oldSkillGain * visibility
    --     end
    -- end
    -- 
    interface = {
        --- Interface version
        -- @field [parent=#SkillProgression] #number version
        version = 0,

        --- Add new skill level up handler for this actor
        -- If `handler(skillid, source, options)` returns false, other handlers (including the default skill level up handler) 
        -- will be skipped. Where skillid and source are the parameters passed to @{SkillProgression#skillLevelUp}, and options is
        -- a modifiable table of skill level up values, and can be modified to change the behavior of later handlers. 
        -- These values are calculated based on vanilla mechanics. Setting any value to nil will cause that mechanic to be skipped. By default contains these values:
        --
        --   * `skillIncreaseValue` - The numeric amount of skill levels gained.
        --   * `levelUpProgress` - The numeric amount of level up progress gained.
        --   * `levelUpAttribute` - The string identifying the attribute that should receive points from this skill level up.
        --   * `levelUpAttributeIncreaseValue` - The numeric amount of attribute increase points received. This contributes to the amount of each attribute the character receives during a vanilla level up.
        --   * `levelUpSpecialization` - The string identifying the specialization that should receive points from this skill level up.
        --   * `levelUpSpecializationIncreaseValue` - The numeric amount of specialization increase points received. This contributes to the icon displayed at the level up screen during a vanilla level up.
        --
        -- @function [parent=#SkillProgression] addSkillLevelUpHandler
        -- @param #function handler The handler.
        addSkillLevelUpHandler = function(handler)
            skillLevelUpHandlers[#skillLevelUpHandlers + 1] = handler
        end,

        --- Add new skillUsed handler for this actor
        -- If `handler(skillid, useType, options)` returns false, other handlers (including the default skill progress handler) 
        -- will be skipped. Where skillid and useType are the parameters passed to @{SkillProgression#skillUsed},
        -- and options is a modifiable table of skill progression values, and can be modified to change the behavior of later handlers. 
        -- By default contains the single value:
        --
        --   * `skillGain` - The numeric amount of skill progress gained, normalized to the range 0 to 1, where 1 is a full level.
        --
        -- @function [parent=#SkillProgression] addSkillUsedHandler
        -- @param #function handler The handler.
        addSkillUsedHandler = function(handler)
            skillUsedHandlers[#skillUsedHandlers + 1] = handler
        end,
        
        --- Trigger a skill use, activating relevant handlers
        -- @function [parent=#SkillProgression] skillUsed
        -- @param #string skillid The if of the skill that was used
        -- @param #SkillUseType useType A number from 0 to 3 (inclusive) representing the way the skill was used, with each use type having a different skill progression rate. Available use types and its effect is skill specific. See @{SkillProgression#skillUseType}
        -- @param #number scale A number that linearly scales the skill progress received from this use. Defaults to 1.
        skillUsed = skillUsed,

        --- @{#SkillUseType}
        -- @field [parent=#SkillProgression] #SkillUseType SKILL_USE_TYPES Available skill usage types
        SKILL_USE_TYPES = {
            -- These are shared by multiple skills
            Armor_HitByOpponent = 0,
            Block_Success = 0,
            Spellcast_Success = 0,
            Weapon_SuccessfulHit = 0,

            -- Skill-specific use types
            Alchemy_CreatePotion = 0,
            Alchemy_UseIngredient = 1,
            Enchant_Recharge = 0,
            Enchant_UseMagicItem = 1,
            Enchant_CreateMagicItem = 2,
            Enchant_CastOnStrike = 3,
            Acrobatics_Jump = 0,
            Acrobatics_Fall = 1,
            Mercantile_Success = 0,
            Mercantile_Bribe = 1, -- Note: This is bugged in vanilla and is not actually in use.
            Security_DisarmTrap = 0,
            Security_PickLock = 1,
            Sneak_AvoidNotice = 0,
            Sneak_PickPocket = 1,
            Speechcraft_Success = 0,
            Speechcraft_Fail = 1,
            Armorer_Repair = 0,
            Athletics_RunOneSecond = 0,
            Athletics_SwimOneSecond = 0,
        },
        
        --- Trigger a skill level up, activating relevant handlers
        -- @function [parent=#SkillProgression] skillLevelUp
        -- @param #string skillid The id of the skill to level up.
        -- @param #SkillLevelUpSource source The source of the skill increase.
        skillLevelUp = skillLevelUp,
        
        --- @{#SkillLevelUpSource}
        -- @field [parent=#SkillProgression] #SkillLevelUpSource SKILL_INCREASE_SOURCES
        SKILL_INCREASE_SOURCES = {
            Book = 'book',
            Usage = 'usage',
            Trainer = 'trainer',
        },
    },
    engineHandlers = { 
        -- Use the interface in these handlers so any overrides will receive the calls.
        _onSkillUse = function (skillid, useType, scale)
            I.SkillProgression.skillUsed(skillid, useType, scale)
        end,
        _onSkillLevelUp = function (skillid, source)
            I.SkillProgression.skillLevelUp(skillid, source)
        end,
    },
}
