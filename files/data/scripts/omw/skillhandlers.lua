local self = require('openmw.self')
local I = require('openmw.interfaces')
local types = require('openmw.types')
local core = require('openmw.core')
local NPC = require('openmw.types').NPC
local Skill = core.stats.Skill

---
-- Table of skill use types defined by morrowind.
-- Each entry corresponds to an index into the available skill gain values
-- of a @{openmw.core#SkillRecord}
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
-- @field #number Athletics_SwimOneSecond 1

---
-- Table of all existing sources for skill increases. Any sources not listed below will be treated as equal to Trainer.
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

local function shallowCopy(t1)
    local t2 = {}
    for key, value in pairs(t1) do t2[key] = value end
    return t2
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


local function skillUsed(skillid, options)
    if #skillUsedHandlers == 0 then
        -- If there are no handlers, then there won't be any effect, so skip calculations
        return
    end
    
    -- Make a copy so we don't change the caller's table
    options = shallowCopy(options)
    
    -- Compute use value if it was not supplied directly
    if not options.skillGain then
        if not options.useType or options.useType > 3 or options.useType < 0 then
            print('Error: Unknown useType: '..tostring(options.useType))
            return
        end
        local skillStat = NPC.stats.skills[skillid](self)
        local skillRecord = Skill.record(skillid)
        options.skillGain = skillRecord.skillGain[options.useType + 1]

        if options.scale then 
            options.skillGain = options.skillGain * options.scale
        end
    end

    for i = #skillUsedHandlers, 1, -1 do
        if skillUsedHandlers[i](skillid, options) == false then
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
    -- @context local
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
    -- I.SkillProgression.addSkillUsedHandler(function(skillid, params)
    --     if skillid == 'sneak' and params.useType == I.SkillProgression.SKILL_USE_TYPES.Sneak_AvoidNotice then
    --         local activeEffects = Actor.activeEffects(self)
    --         local visibility = activeEffects:getEffect(core.magic.EFFECT_TYPE.Chameleon).magnitude / 100
    --         visibility = visibility + activeEffects:getEffect(core.magic.EFFECT_TYPE.Invisibility).magnitude
    --         visibility = 1 - math.min(1, math.max(0, visibility))
    --         local oldSkillGain = params.skillGain
    --         params.skillGain = oldSkillGain * visibility
    --     end
    -- end)
    -- 
    interface = {
        --- Interface version
        -- @field [parent=#SkillProgression] #number version
        version = 1,

        --- Add new skill level up handler for this actor.
        -- For load order consistency, handlers should be added in the body if your script.
        -- If `handler(skillid, source, options)` returns false, other handlers (including the default skill level up handler) 
        -- will be skipped. Where skillid and source are the parameters passed to @{#SkillProgression.skillLevelUp}, and options is
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

        --- Add new skillUsed handler for this actor.
        -- For load order consistency, handlers should be added in the body of your script.
        -- If `handler(skillid, options)` returns false, other handlers (including the default skill progress handler) 
        -- will be skipped. Where options is a modifiable table of skill progression values, and can be modified to change the behavior of later handlers. 
        -- Contains a `skillGain` value as well as a shallow copy of the options passed to @{#SkillProgression.skillUsed}.
        -- @function [parent=#SkillProgression] addSkillUsedHandler
        -- @param #function handler The handler.
        addSkillUsedHandler = function(handler)
            skillUsedHandlers[#skillUsedHandlers + 1] = handler
        end,
        
        --- Trigger a skill use, activating relevant handlers
        -- @function [parent=#SkillProgression] skillUsed
        -- @param #string skillid The if of the skill that was used
        -- @param options A table of parameters. Must contain one of `skillGain` or `useType`. It's best to always include `useType` if applicable, even if you set `skillGain`, as it may be used
        -- by handlers to make decisions. See the addSkillUsedHandler example at the top of this page.
        --
        --   * `skillGain` - The numeric amount of skill to be gained.
        --   * `useType` - #SkillUseType, A number from 0 to 3 (inclusive) representing the way the skill was used, with each use type having a different skill progression rate. Available use types and its effect is skill specific. See @{#SkillUseType}
        --
        -- And may contain the following optional parameter:
        --
        --   * `scale` - A numeric value used to scale the skill gain. Ignored if the `skillGain` parameter is set.
        --
        -- Note that a copy of this table is passed to skill used handlers, so any parameters passed to this method will also be passed to the handlers. This can be used to provide additional information to
        -- custom handlers when making custom skill progressions.
        --
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
            Athletics_SwimOneSecond = 1,
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
        
        --- Compute the total skill gain required to level up a skill based on its current level, and other modifying factors such as major skills and specialization.
        -- @function [parent=#SkillProgression] getSkillProgressRequirement
        -- @param #string skillid The id of the skill to compute skill progress requirement for
        getSkillProgressRequirement = getSkillProgressRequirement
    },
    engineHandlers = { 
        -- Use the interface in these handlers so any overrides will receive the calls.
        _onSkillUse = function (skillid, useType, scale)
            I.SkillProgression.skillUsed(skillid, {useType = useType, scale = scale})
        end,
        _onSkillLevelUp = function (skillid, source)
            I.SkillProgression.skillLevelUp(skillid, source)
        end,
    },
}
