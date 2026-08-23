local async = require('openmw.async')
local core = require('openmw.core')
local storage = require('openmw.storage')
local I = require('openmw.interfaces')
local types = require('openmw.types')
local Actor = types.Actor
local Creature = types.Creature
local NPC = types.NPC
local Armor = types.Armor

local combatGroup = 'SettingsOMWCombat'

local common = {}

function common.registerSettingsPage()
    I.Settings.registerPage({
      key = 'OMWCombat',
      l10n = 'OMWCombat',
      name = 'Combat',
      description = 'combatSettingsPageDescription',
    })
end

function common.registerSettingsGroup()
    local function boolSetting(key, default)
        return {
            key = key,
            renderer = 'checkbox',
            name = key,
            description = key..'Description',
            default = default,
        }
    end

    I.Settings.registerGroup({
        key = combatGroup,
        page = 'OMWCombat',
        l10n = 'OMWCombat',
        name = 'combatSettings',
        permanentStorage = false,
        order = 0,
        settings = {
            boolSetting('unarmedCreatureAttacksDamageArmor', false),
            boolSetting('redistributeShieldHitsWhenNotWearingShield', false),
            boolSetting('spawnBloodEffectsOnPlayer', false),
        },
    })
end

common.settings = storage.globalSection('SettingsOMWCombat')

common.armorTypeGmst = {
    [Armor.TYPE.Boots] = core.getGMST('iBootsWeight'),
    [Armor.TYPE.Cuirass] = core.getGMST('iCuirassWeight'),
    [Armor.TYPE.Greaves] = core.getGMST('iGreavesWeight'),
    [Armor.TYPE.Helmet] = core.getGMST('iHelmWeight'),
    [Armor.TYPE.LBracer] = core.getGMST('iGauntletWeight'),
    [Armor.TYPE.LGauntlet] = core.getGMST('iGauntletWeight'),
    [Armor.TYPE.LPauldron] = core.getGMST('iPauldronWeight'),
    [Armor.TYPE.RBracer] = core.getGMST('iGauntletWeight'),
    [Armor.TYPE.RGauntlet] = core.getGMST('iGauntletWeight'),
    [Armor.TYPE.RPauldron] = core.getGMST('iPauldronWeight'),
    [Armor.TYPE.Shield] = core.getGMST('iShieldWeight'),
}

common.armorSlots = {
    Actor.EQUIPMENT_SLOT.Boots,
    Actor.EQUIPMENT_SLOT.Cuirass,
    Actor.EQUIPMENT_SLOT.Greaves,
    Actor.EQUIPMENT_SLOT.Helmet,
    Actor.EQUIPMENT_SLOT.LeftGauntlet,
    Actor.EQUIPMENT_SLOT.LeftPauldron,
    Actor.EQUIPMENT_SLOT.RightGauntlet,
    Actor.EQUIPMENT_SLOT.RightPauldron,
    Actor.EQUIPMENT_SLOT.CarriedLeft,
}

function common.getSkill(actor, skillId)
    if Creature.objectIsInstance(actor) then
        local specialization = core.stats.Skill.record(skillId).specialization
        local creatureRecord = Creature.record(actor)
        return creatureRecord[specialization..'Skill']
    else
        return NPC.stats.skills[skillId](actor).modified
    end
end

function common.asRecord(itemOrId)
    if not itemOrId then return end
    if type(itemOrId) == 'string' then
        return Armor.records[itemOrId]
    elseif itemOrId.__type.name == 'ESM::Armor' then
        return itemOrId
    end
    return Armor.records[itemOrId.recordId]
end

function common.getArmorSkill(itemOrId)
    local item = common.asRecord(itemOrId)
    if not item then
        return 'unarmored'
    end
    local weightGmst = common.armorTypeGmst[item.type]
    local epsilon = 0.0005
    if item.weight <= weightGmst * core.getGMST('fLightMaxMod') + epsilon then
        return 'lightarmor'
    elseif item.weight <= weightGmst * core.getGMST('fMedMaxMod') + epsilon then
        return 'mediumarmor'
    else
        return 'heavyarmor'
    end
end

function common.getArmorRating(actor)
    local magicShield = Actor.activeEffects(actor):getEffect(core.magic.EFFECT_TYPE.Shield).magnitude

    if Creature.objectIsInstance(actor) then
        return magicShield
    end

    local equipment = Actor.getEquipment(actor)
    local ratings = {}
    local unarmored = common.getSkill(actor, 'unarmored')
    local fUnarmoredBase1 = core.getGMST('fUnarmoredBase1')
    local fUnarmoredBase2 = core.getGMST('fUnarmoredBase2')

    for _, v in pairs(common.armorSlots) do
        if equipment[v] and Armor.objectIsInstance(equipment[v]) then
            ratings[v] = I.Combat.getEffectiveArmorRating(equipment[v], actor)
        else
            -- Unarmored
            ratings[v] = (fUnarmoredBase1 * unarmored) * (fUnarmoredBase2 * unarmored)
        end
    end

    return ratings[Actor.EQUIPMENT_SLOT.Cuirass] * 0.3
        +  ratings[Actor.EQUIPMENT_SLOT.CarriedLeft] * 0.1
        +  ratings[Actor.EQUIPMENT_SLOT.Helmet] * 0.1
        +  ratings[Actor.EQUIPMENT_SLOT.Greaves] * 0.1
        +  ratings[Actor.EQUIPMENT_SLOT.Boots] * 0.1
        +  ratings[Actor.EQUIPMENT_SLOT.LeftPauldron] * 0.1
        +  ratings[Actor.EQUIPMENT_SLOT.RightPauldron] * 0.1
        +  ratings[Actor.EQUIPMENT_SLOT.LeftGauntlet] * 0.05
        +  ratings[Actor.EQUIPMENT_SLOT.RightGauntlet] * 0.05
        +  magicShield
end

function common.getSkillAdjustedArmorRating(itemOrId, actor)
    local item = common.asRecord(itemOrId)
    local skillid = I.Combat.getArmorSkill(item)
    local skill = common.getSkill(actor, skillid)
    if item.weight == 0 then
        return item.baseArmor
    end
    return item.baseArmor * skill / core.getGMST('iBaseArmorSkill')
end

function common.getEffectiveArmorRating(item, actor)
    local record = Armor.record(item)
    local rating = I.Combat.getSkillAdjustedArmorRating(record, actor)
    if record.health and record.health ~= 0 then
        rating = rating * (types.Item.itemData(item).condition / record.health)
    end
    return rating
end

function common.adjustDamageForArmor(damage, actor)
    local armor = I.Combat.getArmorRating(actor)
    local x = damage / (damage + armor)
    return damage * math.max(x, core.getGMST('fCombatArmorMinMult'))
end

function common.pickRandomArmor(actor)
    local slot = nil
    local roll = math.random(0, 99) -- randIntUniform(0, 100)
    if roll >= 90 then
        slot = Actor.EQUIPMENT_SLOT.CarriedLeft
        local item = Actor.getEquipment(actor, slot)
        local haveShield = item and Armor.objectIsInstance(item)
        if common.settings:get('redistributeShieldHitsWhenNotWearingShield') and not haveShield then
            if roll >= 95 then
                slot = Actor.EQUIPMENT_SLOT.Cuirass
            else
                slot = Actor.EQUIPMENT_SLOT.LeftPauldron
            end
        end
    elseif roll >= 85 then
        slot = Actor.EQUIPMENT_SLOT.RightGauntlet
    elseif roll >= 80 then
        slot = Actor.EQUIPMENT_SLOT.LeftGauntlet
    elseif roll >= 70 then
        slot = Actor.EQUIPMENT_SLOT.RightPauldron
    elseif roll >= 60 then
        slot = Actor.EQUIPMENT_SLOT.LeftPauldron
    elseif roll >= 50 then
        slot = Actor.EQUIPMENT_SLOT.Boots
    elseif roll >= 40 then
        slot = Actor.EQUIPMENT_SLOT.Greaves
    elseif roll >= 30 then
        slot = Actor.EQUIPMENT_SLOT.Helmet
    else
        slot = Actor.EQUIPMENT_SLOT.Cuirass
    end

    return Actor.getEquipment(actor, slot)
end

function common.getDamage(attack, what)
    if attack.damage then
        return attack.damage[what] or 0
    end
end

function common.setDamage(attack, what, damage)
    attack.damage = attack.damage or {}
    attack.damage[what] = damage
end

function common.hasDamage(attack)
    if attack.damage then
        for _, v in pairs(attack.damage) do
            if v >= 0.001 then
                return true
            end
        end
    end
    return false
end

function common.adjustDamageForDifficulty(attack, defendant)
    local attackerIsPlayer = attack.attacker and types.Player.objectIsInstance(attack.attacker)
    -- The interface guarantees defendant is never nil
    local defendantIsPlayer = types.Player.objectIsInstance(defendant)
    -- If both characters are NPCs or both characters are players then
    -- difficulty settings do not apply
    if attackerIsPlayer == defendantIsPlayer then return end

    local fDifficultyMult = core.getGMST('fDifficultyMult')
    local difficultyTerm = core.getGameDifficulty() * 0.01
    local x = 0

    if defendantIsPlayer then
        -- Defending actor is a player
        if difficultyTerm > 0 then
            x = difficultyTerm * fDifficultyMult
        else
            x = difficultyTerm / fDifficultyMult
        end
    elseif attackerIsPlayer then
        -- Attacking actor is a player
        if difficultyTerm > 0 then
            x = -difficultyTerm / fDifficultyMult
        else
            x = -difficultyTerm * fDifficultyMult
        end
    end

    common.setDamage(attack, 'health', common.getDamage(attack, 'health') * (1 + x))
end

return common
