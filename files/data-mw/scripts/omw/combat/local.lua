local core = require('openmw.core')
local I = require('openmw.interfaces')
local self = require('openmw.self')
local storage = require('openmw.storage')
local types = require('openmw.types')
local Actor = types.Actor
local Player = types.Player
local Creature = types.Creature
local Armor = types.Armor
local auxUtil = require('openmw_aux.util')
local isPlayer = Player.objectIsInstance(self)

local godMode = function() return false end
if isPlayer then
    -- openmw.debug is only allowed on player scripts
    godMode = function() return require('openmw.debug').isGodMode() end
end

local settings = storage.globalSection('SettingsOMWCombat')

local function getSkill(actor, skillId)
    if Creature.objectIsInstance(actor) then
        local specialization = core.stats.Skill.record(skillId).specialization
        local creatureRecord = Creature.record(actor)
        return creatureRecord[specialization..'Skill']
    else
        return types.NPC.stats.skills[skillId](actor).modified
    end
end

local armorTypeGmst = {
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

local armorSlots = {
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

local function getDamage(attack, what)
    if attack.damage then
        return attack.damage[what] or 0
    end
end

local function setDamage(attack, what, damage)
    attack.damage = attack.damage or {}
    attack.damage[what] = damage
end

local function adjustDamageForArmor(damage, actor)
    local armor = I.Combat.getArmorRating(actor)
    local x = damage / (damage + armor)
    return damage * math.max(x, core.getGMST('fCombatArmorMinMult'))
end

local function adjustDamageForDifficulty(attack, defendant)
    local attackerIsPlayer = attack.attacker and Player.objectIsInstance(attack.attacker)
    -- The interface guarantees defendant is never nil
    local defendantIsPlayer = Player.objectIsInstance(defendant)
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

    setDamage(attack, 'health', getDamage(attack, 'health') * (1 + x))
end

local function applyArmor(attack)
    local healthDamage = getDamage(attack, 'health')
    if healthDamage > 0 then
        local healthDamageAdjusted = I.Combat.adjustDamageForArmor(healthDamage)
        local diff = math.floor(healthDamageAdjusted - healthDamage)
        setDamage(attack, 'health', math.max(healthDamageAdjusted, 1))
        local item = I.Combat.pickRandomArmor()
        local skillid = I.Combat.getArmorSkill(item)
        if I.SkillProgression then
            I.SkillProgression.skillUsed(skillid, {useType = I.SkillProgression.SKILL_USE_TYPES.Armor_HitByOpponent})
        end
        if item and Armor.objectIsInstance(item) then
            local attackerIsUnarmedCreature = attack.attacker and not attack.weapon and not attack.ammo and Creature.objectIsInstance(attack.attacker)
            if settings:get('unarmedCreatureAttacksDamageArmor') or not attackerIsUnarmedCreature then
                core.sendGlobalEvent('ModifyItemCondition', { actor = self, item = item, amount = diff })
            end

            if skillid == 'lightarmor' then
                core.sound.playSound3d('Light Armor Hit', self)
            elseif skillid == 'mediumarmor' then
                core.sound.playSound3d('Medium Armor Hit', self)
            elseif skillid == 'heavyarmor' then
                core.sound.playSound3d('Heavy Armor Hit', self)
            else
                core.sound.playSound3d('Hand To Hand Hit', self)
            end
        end
    end
end

local function getArmorRating(actor)
    local magicShield = Actor.activeEffects(actor):getEffect(core.magic.EFFECT_TYPE.Shield).magnitude

    if Creature.objectIsInstance(actor) then
        return magicShield
    end

    local equipment = Actor.getEquipment(actor)
    local ratings = {}
    local unarmored = getSkill(actor, 'unarmored')
    local fUnarmoredBase1 = core.getGMST('fUnarmoredBase1')
    local fUnarmoredBase2 = core.getGMST('fUnarmoredBase2')

    for _, v in pairs(armorSlots) do
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

local function getArmorSkill(item)
    if not item or not Armor.objectIsInstance(item) then
        return 'unarmored'
    end
    local record = Armor.record(item)
    local weightGmst = armorTypeGmst[record.type]
    local epsilon = 0.0005
    if record.weight <= weightGmst * core.getGMST('fLightMaxMod') + epsilon then
        return 'lightarmor'
    elseif record.weight <= weightGmst * core.getGMST('fMedMaxMod') + epsilon then
        return 'mediumarmor'
    else
        return 'heavyarmor'
    end
end

local function getSkillAdjustedArmorRating(item, actor)
    local record = Armor.record(item)
    local skillid = I.Combat.getArmorSkill(item)
    local skill = getSkill(actor, skillid)
    if record.weight == 0 then
        return record.baseArmor
    end
    return record.baseArmor * skill / core.getGMST('iBaseArmorSkill')
end

local function getEffectiveArmorRating(item, actor)
    local record = Armor.record(item)
    local rating = getSkillAdjustedArmorRating(item, actor)
    if record.health and record.health ~= 0 then
        rating = rating * (types.Item.itemData(item).condition / record.health)
    end
    return rating
end

local function spawnBloodEffect(position)
    if isPlayer and not settings:get('spawnBloodEffectsOnPlayer') then
        return
    end

    local bloodEffectModel = string.format('Blood_Model_%d', math.random(0, 2)) -- randIntUniformClosed(0, 2)

    -- TODO: implement a Misc::correctMeshPath equivalent instead?
    -- All it ever does it append 'meshes\\' though
    bloodEffectModel = 'meshes/'..core.getGMST(bloodEffectModel)

    local record = self.object.type.record(self.object)
    local bloodTexture = string.format('Blood_Texture_%d', record.bloodType)
    bloodTexture = core.getGMST(bloodTexture)
    if not bloodTexture or bloodTexture == '' then
        bloodTexture = core.getGMST('Blood_Texture_0')
    end
    core.sendGlobalEvent('SpawnVfx', {
        model = bloodEffectModel,
        position = position,
        options = {
            mwMagicVfx = false,
            particleTextureOverride = bloodTexture,
            useAmbientLight = false,
        },
    })
end

local function pickRandomArmor(actor)
    local slot = nil
    local roll = math.random(0, 99) -- randIntUniform(0, 100)
    if roll >= 90 then
        slot = Actor.EQUIPMENT_SLOT.CarriedLeft
        local item = Actor.getEquipment(actor, slot)
        local haveShield = item and Armor.objectIsInstance(item)
        if settings:get('redistributeShieldHitsWhenNotWearingShield') and not haveShield then
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

local function onHit(data)
    if data.successful and not godMode() then
        I.Combat.applyArmor(data)
        I.Combat.adjustDamageForDifficulty(data)
        if getDamage(data, 'health') > 0 then
            core.sound.playSound3d('Health Damage', self)
            if data.hitPos then
                I.Combat.spawnBloodEffect(data.hitPos)
            end
        end
    elseif data.attacker and Player.objectIsInstance(data.attacker) then
        core.sound.playSound3d('miss', self)
    end
    Actor._onHit(self, data)
end

I.Combat.addOnHitHandler(onHit)

local interface = auxUtil.shallowCopy(I.Combat)
interface.adjustDamageForArmor = function(damage, actor) return adjustDamageForArmor(damage, actor or self) end
interface.adjustDamageForDifficulty = function(attack, defendant) return adjustDamageForDifficulty(attack, defendant or self) end
interface.applyArmor = applyArmor
interface.getArmorRating = function(actor) return getArmorRating(actor or self) end
interface.getArmorSkill = getArmorSkill
interface.getSkillAdjustedArmorRating = function(item, actor) return getSkillAdjustedArmorRating(item, actor or self) end
interface.getEffectiveArmorRating = function(item, actor) return getEffectiveArmorRating(item, actor or self) end
interface.spawnBloodEffect = spawnBloodEffect
interface.pickRandomArmor = function(actor) return pickRandomArmor(actor or self) end

return {
    interfaceName = 'Combat',
    interface = interface
}
